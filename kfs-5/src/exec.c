#include "exec.h"
#include "sched.h"
#include "pmm.h"
#include "hmm.h"
#include "gdt.h"
#include "utils.h"
#include "panic.h"
#include "pid.h"

static inline void __user_vblock_tree_clear(void)
{
    struct user_vblock *cur, *tmp;
    struct rb_root *root;

    root = &current->vblocks.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        __rb_erase(&cur->by_base, root);
        kfree(cur);
    }
    root = &current->vblocks.by_size;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_size) {
        __rb_erase(&cur->by_size, root);
        kfree(cur);
    }
}

static inline void __mapping_file_tree_clear(void)
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    uint32_t *pte;
    uintptr_t addr;
    size_t size;

    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        addr = cur->base;
        size = cur->size;
        pte = (uint32_t *)pte_from_addr(cur->base);
        do {
            if (page_is_present(*pte)) {
                tlb_flush(addr);
                free_pages(pfn_from_pte(*pte), PAGE_SIZE);
            }
            pte++;
            size -= PAGE_SIZE;
            addr += PAGE_SIZE;
        } while (size);
        __rb_erase(&cur->by_base, root);
        kfree(cur);
    }
}

static inline void __tree_clear(void)
{
    __mapping_file_tree_clear();
    __user_vblock_tree_clear();
}

static inline void __page_dir_clear(void)
{
    uint32_t *pde;
    uintptr_t addr;

    pde = (uint32_t *)PAGE_DIR;
    for (size_t i = 0; i < 768; i++) {
        if (pde[i]) {
            addr = PAGE_TAB + (i*PAGE_SIZE);
            tlb_flush(addr);
            free_pages(pfn_from_pde20(pde[i]), PAGE_SIZE);
        }
    }
    memset32(pde, 0, 768);
}

static inline void __page_dir_init(void)
{
    uint32_t *pde, *pte;
    uintptr_t page;
    uint32_t pde_idx, pte_idx;

    pde = (uint32_t *)PAGE_DIR;
    pte = (uint32_t *)PAGE_TAB;
    
    pde_idx = USER_CODE_BASE >> 22;
    pte_idx = (USER_CODE_BASE >> 12) & 0x3FF;
    page = alloc_pages(PAGE_SIZE);
    pde[pde_idx] = page | PG_US | PG_RDWR | PG_PRESENT;
    page = alloc_pages(PAGE_SIZE);
    pte[pte_idx + pde_idx * 1024] = page | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_CODE_SIZE / PAGE_SIZE); i++)
        pte[pte_idx + pde_idx * 1024 + i] = PG_RESERVED | PG_US | PG_RDWR;

    pde_idx = USER_STACK_BASE >> 22;
    pte_idx = (USER_STACK_TOP >> 12) & 0x3FF;
    page = alloc_pages(PAGE_SIZE);
    pde[pde_idx] = page | PG_US | PG_RDWR | PG_PRESENT;
    page = alloc_pages(PAGE_SIZE);
    pte[pte_idx + pde_idx * 1024] = page | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_STACK_SIZE / PAGE_SIZE); i++)
        pte[pte_idx + pde_idx * 1024 - i] = PG_RESERVED | PG_US | PG_RDWR;
}


static inline void __mapping_file_tree_init(void)
{
    struct rb_root *root;
    struct rb_node **cur, *parent; 
    struct mapping_file *mfile;

    mfile = (struct mapping_file *)kmalloc(sizeof(struct mapping_file));
    mfile->base = USER_CODE_BASE;
    mfile->size = USER_CODE_SIZE;

    root = &current->mapping_files.by_base;
    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (mfile->base < rb_entry(parent, struct mapping_file, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&mfile->by_base, parent, cur);
    rb_insert_color(&mfile->by_base, root);
    mfile = (struct mapping_file *)kmalloc(sizeof(struct mapping_file));
    mfile->base = USER_STACK_BASE;
    mfile->size = USER_STACK_SIZE;
}

static inline void __user_vblock_tree_init(void)
{
    struct rb_root *root;
    struct rb_node **cur, *parent; 
    struct user_vblock *vblock;

    vblock = (struct user_vblock *)kmalloc(sizeof(struct user_vblock));
    vblock->base = USER_CODE_BASE + USER_CODE_SIZE;
    vblock->size = USER_STACK_BASE - vblock->base;

    root = &current->vblocks.by_base;
    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (vblock->base < rb_entry(parent, struct user_vblock, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&vblock->by_base, parent, cur);
    rb_insert_color(&vblock->by_base, root);

    root = &current->vblocks.by_size;
    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (vblock->size < rb_entry(parent, struct user_vblock, by_size)->size)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&vblock->by_size, parent, cur);
    rb_insert_color(&vblock->by_size, root);
}

static inline void __tree_init(void)
{
    __mapping_file_tree_init();
    __user_vblock_tree_init();
}

static inline void __exec(uintptr_t user_eip) {
    asm volatile (
        "mov %[user_ds], %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "pushl %[user_ss]\n"
        "pushl %[user_esp]\n"
        "pushf\n"
        "orl $0x200, (%%esp)\n"
        "pushl %[user_cs]\n"
        "pushl %[user_eip]\n"
        "iret\n"
        :
        : [user_ss] "i" (GDT_SELECTOR_DATA_PL3),
          [user_esp] "i" (USER_STACK_TOP),
          [user_cs] "i" (GDT_SELECTOR_CODE_PL3),
          [user_eip] "r" (user_eip),
          [user_ds] "i" (GDT_SELECTOR_DATA_PL3)
        : "memory", "eax"
    );
}

static inline void __ts_init(struct task_struct *ts)
{
	ts->pid = alloc_pid();
	ts->parent = NULL;
	ts->vblocks.by_base = RB_ROOT;
	ts->vblocks.by_size = RB_ROOT;
	ts->mapping_files.by_base = RB_ROOT;
	init_list_head(&ts->child);
}

static void test_user_code(void)
{
    int ret;

    while (42) {
        asm volatile (
            "int $0x80"
            : "=a"(ret)
            : "a"(0), "b"("syscall test!!")
            : "memory"
        );
    }
}

void init_process(void)
{
	struct task_struct *ts;

	ts = kmalloc(sizeof(struct task_struct));
	if (!ts)
		do_panic("init process create failed");
    __ts_init(ts);
    current = ts;
    exec_fn(test_user_code);
}

void exec_fn(void (*func)())
{
    __tree_clear();
    __page_dir_clear();
    __tree_init();
    __page_dir_init();
    memcpy((void *)USER_CODE_BASE, func, PAGE_SIZE);
    __exec(USER_CODE_BASE);
    __builtin_unreachable();
}