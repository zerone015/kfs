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
#include "errno.h"

uint16_t *page_ref;

static inline void page_ref_init(void)
{
    size_t page_ref_size;

    page_ref_size = ram_size / PAGE_SIZE * sizeof(uint16_t);
    page_ref = kmalloc(page_ref_size);
    if (!page_ref)
        do_panic("scheduler_init failed");
    memset(page_ref, 0, page_ref_size);
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

void proc_init(void)
{
    page_ref_init();
}

void init_process(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(struct task_struct));
	if (!task)
		do_panic("init process create failed");

    task->pid = alloc_pid();
    if (task->pid == PID_NONE)
		do_panic("init process create failed");

    task->parent = NULL;
    task->time_slice_remaining = DEFAULT_TIMESLICE;
	task->vblocks.by_base = RB_ROOT;
	task->vblocks.by_size = RB_ROOT;
	task->mapping_files.by_base = RB_ROOT;
	init_list_head(&task->child_list);

    current = task;

    exec_fn(test_user_code);
}

static inline void mapping_files_clear_only_node(struct mapping_file_tree *mapping_files)
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;

    root = &mapping_files->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        kfree(cur);
    }
}

static inline void entries_clean(void)
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    size_t size, pfn;
    uint32_t *pte;
    
    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        size = cur->size;
        pte = (uint32_t *)pte_from_addr(cur->base);
        do {
            pfn = pfn_from_pte(*pte);
            if (is_cow(*pte)) {
                if (page_ref[pfn] == 2) {
                    page_ref[pfn] = 0;
                    *pte = *pte & ~(PG_COW_RDONLY | PG_COW_RDWR);
                    if (!is_code_section(addr_from_pte(pte)))
                        *pte |= PG_RDWR;
                }
                else {
                    page_ref[pfn]--;
                }
            }
            pte++;
            size -= PAGE_SIZE;
        } while (size);
    }
}

static inline void entries_set_cow(void) 
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    size_t size, pfn;
    uint32_t *pte;
    
    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        size = cur->size;
        pte = (uint32_t *)pte_from_addr(cur->base);
        do {
            pfn = pfn_from_pte(*pte);
            if (is_cow(*pte)) {
                page_ref[pfn]++;
            } 
            else if (page_is_present(*pte)) {
                if (is_code_section(addr_from_pte(pte))) {
                    *pte |= PG_COW_RDONLY;
                }
                else {
                    *pte = (*pte | PG_COW_RDWR) & ~PG_RDWR;
                    tlb_flush(addr_from_pte(pte));
                }
                page_ref[pfn] = 2;
            }
            pte++;
            size -= PAGE_SIZE;
        } while (size);
    }
}

static inline void pgdir_clean(uint32_t *pde, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (pde[i])
            free_pages(page_from_pde_4kb(pde[i]), PAGE_SIZE);
    }
    vb_unmap(pde);
}

static inline uintptr_t pgdir_clone(void)
{
    uint32_t *pde, *pte, *pgtab;
    uintptr_t pde_page;
    size_t i;

    pde = (uint32_t *)vb_alloc(PAGE_SIZE);
    if (!pde)
        goto out;
    memcpy32(pde, (void *)PAGE_DIR, PAGE_SIZE / 4);
    for (i = 0; i < 768; i++) {
        if (pde[i]) {
            pgtab = (uint32_t *)vb_alloc(PAGE_SIZE);
            if (!pgtab) 
                goto out_clean_pgdir;
            memcpy32(pgtab, (void *)(PAGE_TAB + i*PAGE_SIZE), PAGE_SIZE / 4);
            pte = (uint32_t *)pte_from_addr(pgtab);
            pde[i] = page_from_pte(*pte) | flags_from_entry(pde[i]);
            vb_unmap(pgtab);
        }
    }
    pte = (uint32_t *)pte_from_addr(pde);
    pde_page = page_from_pte(*pte);
    pde[1023] = pde_page | flags_from_entry(pde[1023]);
    vb_unmap(pde);
    return pde_page;

out_clean_pgdir:
    pgdir_clean(pde, i);
out:
    return PAGE_NONE;
}

static inline int vblocks_clone(struct user_vblock_tree *vblocks)
{
    struct user_vblock *cur, *tmp, *new;
    struct rb_root *root;

    root = &current->vblocks.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        new = kmalloc(sizeof(struct user_vblock));
        if (!new)
            return -1;
        new->base = cur->base;
        new->size = cur->size;
        vblock_bybase_add(new, &vblocks->by_base);
        vblock_bysize_add(new, &vblocks->by_size);
    }
    return 0;
}

static inline int mapping_files_clone(struct mapping_file_tree *mapping_files)
{
    struct mapping_file *cur, *tmp, *new;
    struct rb_root *root;

    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        new = kmalloc(sizeof(struct mapping_file));
        if (!new)
            return -1;
        new->base = cur->base;
        new->size = cur->size;
        mapping_file_add(new, &mapping_files->by_base);
    }
    return 0;
}

static inline int create_task(struct interrupt_frame *iframe, struct task_struct **out_task)
{
    struct task_struct *task;
    int ret = -ENOMEM;

    task = kmalloc(sizeof(struct task_struct));
    if (!task)
        goto out;

    task->pid = alloc_pid();
    if (task->pid == PID_NONE) {
        ret = -EAGAIN;
        goto out_clean_task;
    }
    
    task->vblocks.by_base = RB_ROOT;
    task->vblocks.by_size = RB_ROOT;
    if (vblocks_clone(&task->vblocks) < 0)
        goto out_clean_pid;
    
    task->mapping_files.by_base = RB_ROOT;
    if (mapping_files_clone(&task->mapping_files) < 0)
        goto out_clean_vblocks;

    entries_set_cow();
    task->cr3 = pgdir_clone();
    if (task->cr3 == PAGE_NONE)
        goto out_clean_mapping_files;
    
    task->time_slice_remaining = DEFAULT_TIMESLICE;
    task->parent = current;
    task->state = PROCESS_READY;

    task->cpu_context = (struct cpu_context) {
        .eax = 0,
        .ebp = iframe->ebp,
        .ebx = iframe->ebx,
        .ecx = iframe->ecx,
        .edi = iframe->edi,
        .edx = iframe->edx,
        .esi = iframe->esi,
        .esp = iframe->esp,
        .eip = iframe->eip,
        .eflags = iframe->eflags,
    };
    init_list_head(&task->child_list);

    *out_task = task;
    return 0;

out_clean_mapping_files:
    mapping_files_clear_only_node(&task->mapping_files);
    entries_clean();
out_clean_vblocks:
    vblocks_clear(&task->vblocks);
out_clean_pid:
    free_pid(task->pid);
out_clean_task:
    kfree(task);
out:
    return ret;
}

int fork(struct syscall_frame *sframe)
{
    struct task_struct *task;
    int ret;

    ret = create_task(&sframe->iframe, &task);
    if (ret < 0)
        return ret;
    list_add(&task->child, &current->child_list);
    ready_queue_enqueue(task);
    return task->pid;
}