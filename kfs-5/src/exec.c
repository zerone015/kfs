#include "exec.h"
#include "sched.h"
#include "pmm.h"
#include "hmm.h"
#include "gdt.h"
#include "utils.h"
#include "panic.h"
#include "pid.h"
#include "proc.h"

static inline void mapping_files_clear(struct mapping_file_tree *mapping_files)
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    uint32_t *pte;
    size_t size, pfn;

    root = &mapping_files->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        size = cur->size;
        pte = (uint32_t *)pte_from_addr(cur->base);
        do {
            if (is_cow(*pte)) {
                pfn = pfn_from_pte(*pte);
                if (page_ref[pfn] == 1)
                    free_pages(page_from_pfn(pfn), PAGE_SIZE);
                page_ref[pfn]--;
            } 
            else if (page_is_present(*pte)) {
                free_pages(page_from_pte(*pte), PAGE_SIZE);
            }
            tlb_flush(addr_from_pte(pte));
            pte++;
            size -= PAGE_SIZE;
        } while (size);
        kfree(cur);
    }
    root->rb_node = NULL;
}

static inline void vspace_clear(void)
{
    vblocks_clear(&current->vblocks);
    mapping_files_clear(&current->mapping_files);
}

static inline void page_dir_clear(void)
{
    uint32_t *pde;
    uintptr_t pgtab;

    pde = (uint32_t *)PAGE_DIR;
    for (size_t i = 0; i < 768; i++) {
        if (pde[i]) {
            free_pages(page_from_pde_4kb(pde[i]), PAGE_SIZE);
            pgtab = PAGE_TAB + (i*PAGE_SIZE);
            tlb_flush(pgtab);
        }
    }
    memset32(pde, 0, 768);
}

static inline void page_dir_init(void)
{
    uint32_t *pde, *pte;

    pde = (uint32_t *)PAGE_DIR;
    pte = (uint32_t *)PAGE_TAB;
    
    pde[pde_idx(USER_CODE_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    pte[pte_idx(USER_CODE_BASE) + pde_idx(USER_CODE_BASE) * 1024] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_CODE_SIZE / PAGE_SIZE); i++)
        pte[pte_idx(USER_CODE_BASE) + 1024*pde_idx(USER_CODE_BASE) + i] = PG_RESERVED | PG_US | PG_RDWR;

    pde[pde_idx(USER_STACK_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    pte[pte_idx(USER_STACK_TOP) + 1024*pde_idx(USER_STACK_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_STACK_SIZE / PAGE_SIZE); i++)
        pte[pte_idx(USER_STACK_TOP) + 1024*pde_idx(USER_STACK_BASE) - i] = PG_RESERVED | PG_US | PG_RDWR;
}

static inline void mapping_file_tree_init(void)
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

static inline void vblock_tree_init(void)
{
    struct rb_root *root;
    struct user_vblock *new;

    new = (struct user_vblock *)kmalloc(sizeof(struct user_vblock));
    new->base = USER_CODE_BASE + USER_CODE_SIZE;
    new->size = USER_STACK_BASE - new->base - USER_STACK_GUARD_SIZE;

    root = &current->vblocks.by_base;
    vblock_bybase_add(new, root);

    root = &current->vblocks.by_size;
    vblock_bysize_add(new, root);
}

static inline void vspace_init(void)
{
    mapping_file_tree_init();
    vblock_tree_init();
}

static inline void jmp_entry_point(uintptr_t user_eip) {
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

void exec_fn(void (*func)())
{
    vspace_clear();
    page_dir_clear();
    vspace_init();
    page_dir_init();
    memcpy32((void *)USER_CODE_BASE, func, PAGE_SIZE / 4);
    jmp_entry_point(USER_CODE_BASE);
    __builtin_unreachable();
}