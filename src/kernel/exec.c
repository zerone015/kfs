#include "exec.h"
#include "sched.h"
#include "pmm.h"
#include "kmalloc.h"
#include "gdt.h"
#include "utils.h"
#include "panic.h"
#include "pid.h"
#include "init.h"
#include "signal.h"
#include "tmp_syscall.h"

static void pgtab_init(void)
{
    int code_pdi, stack_pdi;
    uint32_t *pgdir, *pgtab;

    pgdir = pgdir_base();
    
    code_pdi = pgdir_index(USER_CODE_BASE);
    stack_pdi = pgdir_index(USER_STACK_BASE);

    pgdir[code_pdi] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    pgdir[stack_pdi] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    
    memset32(pgtab_from_pdi(code_pdi), 0, PAGE_SIZE / 4);
    memset32(pgtab_from_pdi(stack_pdi), 0, PAGE_SIZE / 4);

    pgtab = pte_from_va(USER_CODE_BASE);
    *pgtab = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;
    for (size_t i = 1; i < (USER_CODE_SIZE / PAGE_SIZE); i++)
        pgtab[i] = PG_RESERVED | PG_US | PG_RDWR;
    
    pgtab = pte_from_va(USER_STACK_BASE);
    for (size_t i = 0; i < (USER_STACK_SIZE / PAGE_SIZE); i++)
        pgtab[i] = PG_RESERVED | PG_US | PG_RDWR;
}

static void mapped_vblock_tree_init(void)
{
    struct rb_root *root;
    struct mapped_vblock *new;

    root = &current->mapped_vblocks.by_base;

    new = (struct mapped_vblock *)kmalloc(sizeof(struct mapped_vblock));
    new->base = USER_CODE_BASE;
    new->size = USER_CODE_SIZE;
    mapped_vblock_add(new, root);

    new = (struct mapped_vblock *)kmalloc(sizeof(struct mapped_vblock));
    new->base = USER_STACK_BASE;
    new->size = USER_STACK_SIZE;
    mapped_vblock_add(new, root);
}

static void vblock_tree_init(void)
{
    struct rb_root *root;
    struct u_vblock *new;

    new = (struct u_vblock *)kmalloc(sizeof(struct u_vblock));
    new->base = USER_CODE_BASE + USER_CODE_SIZE;
    new->size = USER_STACK_BASE - new->base - USER_STACK_GUARD_SIZE;

    root = &current->vblocks.by_base;
    vblock_bybase_add(new, root);

    root = &current->vblocks.by_size;
    vblock_bysize_add(new, root);
}

static void cleanup_for_exec(void)
{
    user_vas_cleanup(&current->vblocks, &current->mapped_vblocks, 
                     CL_MAPPING_FREE | CL_TLB_INVL | CL_RECYCLE);
    current->sig_pending = 0;
    signal_init(current->sig_handlers);
}

static void readonly_segment_protect(void)
{
    uint32_t *pte;

    pte = pte_from_va(USER_CODE_BASE);
    for (size_t i = 0; i < (USER_CODE_SIZE / PAGE_SIZE); i++) {
        pte[i] &= ~PG_RDWR;
        tlb_invl(USER_CODE_BASE + i*PAGE_SIZE); 
    }
}

static void vas_init(void (*func)())
{
    mapped_vblock_tree_init();
    vblock_tree_init();
    pgtab_init();

    memcpy((void *)USER_CODE_BASE, func, USER_CODE_SIZE);

    readonly_segment_protect();
}

static void enter_user_mode(void) 
{
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
          [user_eip] "i" (USER_CODE_BASE),
          [user_ds] "i" (GDT_SELECTOR_DATA_PL3)
        : "memory", "eax"
    );
}

void exec_fn(void (*func)())
{
    cleanup_for_exec();
    vas_init(func);
    enter_user_mode();
}