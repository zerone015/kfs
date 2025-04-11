#include "exec.h"
#include "sched.h"
#include "pmm.h"
#include "hmm.h"
#include "gdt.h"
#include "utils.h"
#include "panic.h"
#include "pid.h"
#include "proc.h"

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
    uintptr_t base;
    size_t size, pfn;

    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        base = cur->base;
        size = cur->size;
        pte = (uint32_t *)pte_from_addr(cur->base);
        do {
            if (page_is_present(*pte)) {
                if (is_rdwr_cow(*pte)) {
                    pfn = pfn_from_pte(*pte);
                    page_ref[pfn]--;
                } else if (!is_rdonly_cow(*pte)) {
                    free_pages(page_from_pte(*pte), PAGE_SIZE);
                }
                tlb_flush(base);
            }
            pte++;
            size -= PAGE_SIZE;
            base += PAGE_SIZE;
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
    uintptr_t pt_addr;

    pde = (uint32_t *)PAGE_DIR;
    for (size_t i = 0; i < 768; i++) {
        if (pde[i]) {
            pt_addr = PAGE_TAB + (i*PAGE_SIZE);
            tlb_flush(pt_addr);
            free_pages(page_from_pde_4kb(pde[i]), PAGE_SIZE);
        }
    }
    memset32(pde, 0, 768);
}

static inline void __page_dir_init(void)
{
    uint32_t *pde, *pte;
    uintptr_t page;

    pde = (uint32_t *)PAGE_DIR;
    pte = (uint32_t *)PAGE_TAB;
    
    page = alloc_pages(PAGE_SIZE);
    pde[pde_idx(USER_CODE_BASE)] = page | PG_US | PG_RDWR | PG_PRESENT;
    page = alloc_pages(PAGE_SIZE);
    pte[pte_idx(USER_CODE_BASE) + pde_idx(USER_CODE_BASE) * 1024] = page | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_CODE_SIZE / PAGE_SIZE); i++)
        pte[pte_idx(USER_CODE_BASE) + 1024*pde_idx(USER_CODE_BASE) + i] = PG_RESERVED | PG_US | PG_RDWR;

    page = alloc_pages(PAGE_SIZE);
    pde[pde_idx(USER_STACK_BASE)] = page | PG_US | PG_RDWR | PG_PRESENT;
    page = alloc_pages(PAGE_SIZE);
    pte[pte_idx(USER_STACK_TOP) + 1024*pde_idx(USER_STACK_BASE)] = page | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_STACK_SIZE / PAGE_SIZE); i++)
        pte[pte_idx(USER_STACK_TOP) + 1024*pde_idx(USER_STACK_BASE) - i] = PG_RESERVED | PG_US | PG_RDWR;
}

static inline void __mapping_file_tree_init(void)
{
    struct rb_root *root;
    struct mapping_file *new;

    root = &current->mapping_files.by_base;

    new = (struct mapping_file *)kmalloc(sizeof(struct mapping_file));
    new->base = USER_CODE_BASE;
    new->size = USER_CODE_SIZE;
    mapping_file_add(new, root);

    new = (struct mapping_file *)kmalloc(sizeof(struct mapping_file));
    new->base = USER_STACK_BASE;
    new->size = USER_STACK_SIZE;
    mapping_file_add(new, root);
}

static inline void __user_vblock_tree_init(void)
{
    struct rb_root *root;
    struct user_vblock *new;

    root = &current->vblocks.by_base;
    new = (struct user_vblock *)kmalloc(sizeof(struct user_vblock));
    new->base = USER_CODE_BASE + USER_CODE_SIZE;
    new->size = USER_STACK_BASE - new->base;
    vblock_bybase_add(new, root);

    root = &current->vblocks.by_size;
    vblock_bysize_add(new, root);
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

static inline void __task_init(struct task_struct *ts)
{
	ts->pid = alloc_pid();
    ts->time_slice_remaining = 10;
	ts->parent = NULL;
	ts->vblocks.by_base = RB_ROOT;
	ts->vblocks.by_size = RB_ROOT;
	ts->mapping_files.by_base = RB_ROOT;
	init_list_head(&ts->child_list);
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
    __task_init(ts);
    current = ts;
    exec_fn(test_user_code);
}

void exec_fn(void (*func)())
{
    __tree_clear();
    __page_dir_clear();
    __tree_init();
    __page_dir_init();
    memcpy32((void *)USER_CODE_BASE, func, PAGE_SIZE / 4);
    __exec(USER_CODE_BASE);
    __builtin_unreachable();
}