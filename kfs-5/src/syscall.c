#include "syscall.h"
#include "errno.h"
#include "printk.h"
#include "panic.h"
#include "pmm.h"
#include "vmm.h"
#include "hmm.h"
#include "utils.h"
#include "sched.h"
#include "rbtree.h"
#include "pid.h"
#include "errno.h"
#include "exec.h"

static inline void kstack_free(struct task_struct *task)
{
    kfree((void *)(task->esp0 - KERNEL_STACK_SIZE));
}

static inline void forget_original_parent(struct task_struct *task)
{
    task->parent = pid_table_lookup(INIT_PROCESS_PID);
    add_child_to_parent(task->parent, task);
}

static inline void reparent_children(struct task_struct *parent)
{
    struct task_struct *cur;

    list_for_each_entry(cur, &parent->children, child) {
        forget_original_parent(cur);
    }
}

static inline void entries_restore(void)
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

static inline void __pgdir_clean(uint32_t *pde, size_t len)
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
    __pgdir_clean(pde, i);
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

static inline int create_child(struct task_struct **out_child)
{
    struct task_struct *child;
    int ret = -ENOMEM;

    child = kmalloc(sizeof(struct task_struct));
    if (!child)
        goto out;

    child->pid = alloc_pid();
    if (child->pid == PID_NONE) {
        ret = -EAGAIN;
        goto out_clean_child;
    }

    child->esp0 = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!child->esp0)
        goto out_clean_pid;
    child->esp0 += KERNEL_STACK_SIZE;
    
    child->vblocks.by_base = RB_ROOT;
    child->vblocks.by_size = RB_ROOT;
    if (vblocks_clone(&child->vblocks) < 0)
        goto out_clean_kstack;
    
    child->mapping_files.by_base = RB_ROOT;
    if (mapping_files_clone(&child->mapping_files) < 0)
        goto out_clean_vblocks;

    entries_set_cow();
    child->cr3 = pgdir_clone();
    if (child->cr3 == PAGE_NONE)
        goto out_restore_entries;
    
    child->time_slice_remaining = DEFAULT_TIMESLICE;
    child->parent = current;
    child->state = PROCESS_READY;

    init_list_head(&child->children);

    *out_child = child;
    return 0;

out_restore_entries:
    entries_restore();
    mapping_files_clean(&child->mapping_files, false, false);
out_clean_vblocks:
    vblocks_clean(&child->vblocks);
out_clean_kstack:
    kstack_free(child);
out_clean_pid:
    free_pid(child->pid);
out_clean_child:
    kfree(child);
out:
    return ret;
}

static inline int sys_fork(void)
{
    struct task_struct *child;
    int ret;

    ret = create_child(&child);
    if (ret < 0)
        return ret;
    stack_copy_and_adjust(child);
    if (current == child)
        return 0;
    add_child_to_parent(current, child);
    pid_table_register(child);
    ready_queue_register(child);
    return child->pid;
}

static inline int sys_wait(int *status)
{
    struct task_struct *child;
    int ret = -ECHILD;

    if (list_empty(&current->children))
        goto out;
    list_for_each_entry(child, &current->children, child) {
        if (child->state == PROCESS_ZOMBIE)
            goto out_clean_child;
    }
    current->state = PROCESS_WAITING_ZOMBIE;
    yield();
    // if signal
    // ret = -EINTR
    // goto out;
    list_for_each_entry(child, &current->children, child) {
        if (child->state == PROCESS_ZOMBIE)
            goto out_clean_child;
    }
    __builtin_unreachable();
out_clean_child:
    kstack_free(child);
    free_pages(child->cr3, PAGE_SIZE);
    free_pid(child->pid);
    remove_child_from_parent(child);
    pid_table_unregister(child->pid);
    *status = child->exit_status;
    ret = child->pid;
    kfree(child);
out:
    return ret;
}

static inline void __attribute__((noreturn)) sys_exit(int status)
{
    user_vspace_clean(&current->vblocks, &current->mapping_files, 
        CL_MAPPING_FREE);
    reparent_children(current);
    current->state = PROCESS_ZOMBIE;
    current->exit_status = status;
    // TODO: SIGCHLD
    if (current->parent->state == PROCESS_WAITING_ZOMBIE) {
        current->parent->state = PROCESS_READY;
        ready_queue_register(current->parent);
    }
    yield();
    __builtin_unreachable();
}

int syscall_dispatch(struct syscall_frame sframe)
{
    int ret = -ENOSYS;

    switch (sframe.syscall_num) {
    case SYS_exit:
        sys_exit(sframe.arg1);
    case SYS_fork:
        ret = sys_fork();
        break;
    case SYS_write:
        printk((const char *)sframe.arg1);
        ret = 0;
        break;
    case SYS_wait:
        ret = sys_wait((int *)sframe.arg1);
        break;
    }
    return ret;
}