#include "exec.h"
#include "sched.h"
#include "pmm.h"
#include "hmm.h"
#include "gdt.h"
#include "utils.h"
#include "panic.h"
#include "pid.h"
#include "proc.h"

static inline void pgdir_init(void)
{
    uint32_t *pde, *pte;

    pde = (uint32_t *)PAGE_DIR;
    pte = (uint32_t *)PAGE_TAB;
    
    pde[pde_idx(USER_CODE_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    pte[pte_idx(USER_CODE_BASE) + 1024*pde_idx(USER_CODE_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_CODE_SIZE / PAGE_SIZE); i++)
        pte[pte_idx(USER_CODE_BASE) + 1024*pde_idx(USER_CODE_BASE) + i] = PG_RESERVED | PG_US | PG_RDWR;

    pde[pde_idx(USER_STACK_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 0; i < ((USER_STACK_SIZE / PAGE_SIZE) - 1); i++)
        pte[pte_idx(USER_STACK_BASE) + 1024*pde_idx(USER_STACK_BASE) + i] = PG_RESERVED | PG_US | PG_RDWR;
    pte[pte_idx(USER_STACK_TOP - 4) + 1024*pde_idx(USER_STACK_BASE)] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
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

static inline void user_vspace_init(void)
{
    mapping_file_tree_init();
    vblock_tree_init();
    pgdir_init();
}

static inline void jmp_entry(uintptr_t user_eip) {
    __asm__ (
        "mov %[user_ds], %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "pushl %[user_ss]\n\t"
        "pushl %[user_esp]\n\t"
        "pushf\n\t"
        "orl $0x200, (%%esp)\n\t"
        "pushl %[user_cs]\n\t"
        "pushl %[user_eip]\n\t"
        "iret"
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
    user_vspace_clean(&current->vblocks, &current->mapping_files, 
        CL_MAPPING_FREE | CL_TLB_FLUSH | CL_RECYCLE);
    user_vspace_init();
    memcpy32((void *)USER_CODE_BASE, func, PAGE_SIZE / 4);
    jmp_entry(USER_CODE_BASE);
}