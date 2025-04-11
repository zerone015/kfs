#include "proc.h"
#include "panic.h"
#include "pmm.h"
#include "vmm.h"
#include "hmm.h"
#include "utils.h"
#include "sched.h"
#include "rbtree.h"
#include "exec.h"
#include "pid.h"
#include "interrupt.h"

uint16_t *page_ref;

static inline void __page_ref_init(void)
{
    size_t page_ref_size;

    page_ref_size = ram_size / PAGE_SIZE * sizeof(uint16_t);
    page_ref = kmalloc(page_ref_size);
    if (!page_ref)
        do_panic("scheduler_init failed");
    memset(page_ref, 0, page_ref_size);
}

void proc_init(void)
{
    __page_ref_init();
}

static inline void __cow_prepare_pages(void) 
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    uintptr_t base;
    size_t size, pfn;
    uint32_t *pte;
    
    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        base = cur->base;
        size = cur->size;
        pte = (uint32_t *)pte_from_addr(base);
        if (base == USER_CODE_BASE) {
            do {
                *pte = (*pte | PG_COW_RDONLY);
                pte++;
                size -= PAGE_SIZE;
            } while (size);
        }
        else {
            do {
                pfn = pfn_from_pte(*pte);
                if (is_rdwr_cow(*pte)) {
                    page_ref[pfn]++;
                } else if (page_is_present(*pte)) {
                    *pte = (*pte | PG_COW_RDWR) & ~PG_RDWR;
                    tlb_flush(base);
                    page_ref[pfn] = 2;
                }
                pte++;
                size -= PAGE_SIZE;
                base += PAGE_SIZE;
            } while (size);
        }
    }
}

static inline uintptr_t __pgdir_clone(void)
{
    uint32_t *pde, *pte, *pgtab;
    uintptr_t ret;

    pde = (uint32_t *)vb_alloc(PAGE_SIZE);
    memcpy32(pde, (void *)PAGE_DIR, PAGE_SIZE / 4);
    for (size_t i = 0; i < 768; i++) {
        if (pde[i]) {
            pgtab = (uint32_t *)vb_alloc(PAGE_SIZE);
            memcpy32(pgtab, (void *)(PAGE_TAB + i*PAGE_SIZE), PAGE_SIZE / 4);
            pte = (uint32_t *)pte_from_addr(pgtab);
            pde[i] = page_from_pte(*pte) | flags_from_entry(pde[i]);
            vb_unmap(pgtab);
        }
    }
    pte = (uint32_t *)pte_from_addr(pde);
    ret = page_from_pte(*pte);
    vb_unmap(pde);
    return ret;
}

static inline void __vblocks_clone(struct user_vblock_tree *vblocks)
{
    struct user_vblock *cur, *tmp, *new;
    struct rb_root *root;

    root = &current->vblocks.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        new = kmalloc(sizeof(struct user_vblock));
        new->base = cur->base;
        new->size = cur->size;
        vblock_bybase_add(new, &vblocks->by_base);
        vblock_bysize_add(new, &vblocks->by_size);
    }
}

static inline void __mapping_files_clone(struct mapping_file_tree *mapping_files)
{
    struct mapping_file *cur, *tmp, *new;
    struct rb_root *root;

    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        new = kmalloc(sizeof(struct mapping_file));
        new->base = cur->base;
        new->size = cur->size;
        mapping_file_add(new, &mapping_files->by_base);
    }
}

static inline void __vspace_tree_clone(struct user_vblock_tree *vblocks, struct mapping_file_tree *mapping_files)
{
    __vblocks_clone(vblocks);
    __mapping_files_clone(mapping_files);
}

static inline struct task_struct * __create_child_task(struct interrupt_frame *iframe)
{
    struct task_struct *ts;

    ts = kmalloc(sizeof(struct task_struct));
    ts->pid = alloc_pid();
    ts->time_slice_remaining = 10;
    ts->parent = current;
    ts->state = PROCESS_READY;
    init_list_head(&ts->child_list);
    ts->context.eax = 0;
    ts->context.ebp = iframe->ebp;
    ts->context.ebx = iframe->ebx;
    ts->context.ecx = iframe->ecx;
    ts->context.edi = iframe->edi;
    ts->context.edx = iframe->edx;
    ts->context.eflags = iframe->eflags;
    ts->context.eip = iframe->eip;
    ts->context.esi = iframe->esi;
    ts->context.esp = iframe->esp;
    ts->context.cr3 = __pgdir_clone();
    ts->mapping_files.by_base = RB_ROOT;
    ts->vblocks.by_base = RB_ROOT;
    ts->vblocks.by_size = RB_ROOT;
    __vspace_tree_clone(&ts->vblocks, &ts->mapping_files);
    return ts;
}

int fork(struct syscall_frame *sframe)
{
    struct task_struct *ts;

    __cow_prepare_pages();
    ts = __create_child_task(&sframe->iframe);
    list_add(&ts->child, &current->child_list);
    task_enqueue(ts);
    return ts->pid;
}