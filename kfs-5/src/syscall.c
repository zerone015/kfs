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

static inline int reap_zombie(struct task_struct *child, int *status)
{
    int ret;
    
    ret = child->pid;
    if (status)
        *status = child->exit_status;

    free_pid(child->pid);
    free_pages(child->cr3, PAGE_SIZE);
    kfree((void *)(child->esp0 - KERNEL_STACK_SIZE));

    remove_child_from_parent(child);
    pid_table_unregister(child->pid);
    
    kfree(child);
    return ret;
}

static inline void pages_set_cow(void) 
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    uint32_t *pte;
    page_t page;
    
    root = &current->mapping_files.by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        pte = pte_from_addr(cur->base);
        for (size_t i = 0; i < (cur->size / PAGE_SIZE); i++) {
            page = page_from_pte(pte[i]);
            if (is_cow(pte[i])) {
                page_ref_inc(page);
            } 
            else if (page_is_present(pte[i])) {
                if (is_code_section(cur->base + (i*PAGE_SIZE))) {
                    pte[i] |= PG_COW_RDONLY;
                }
                else {
                    pte[i] = (pte[i] | PG_COW_RDWR) & ~PG_RDWR;
                    tlb_invl(cur->base + (i*PAGE_SIZE));
                }
                page_set_shared(page);
            }
        }
    }
}

static inline page_t pgdir_clone(void)
{
    uint32_t *pgdir;
    void *pgtab;
    page_t pgdir_page, pgtab_page;

    pgdir_page = alloc_pages(PAGE_SIZE);
    pgdir = (uint32_t *)tmp_vmap(pgdir_page);
    memcpy32(pgdir, current_pgdir(), PAGE_SIZE / 4);
    for (size_t i = 0; i < 768; i++) {
        if (pgdir[i]) {
            pgtab_page = alloc_pages(PAGE_SIZE);
            pgtab = tmp_vmap(pgtab_page);
            memcpy32(pgtab, pgtab_from_pdi(i), PAGE_SIZE / 4);
            tmp_vunmap(pgtab);
            pgdir[i] = pgtab_page | pg_entry_flags(pgdir[i]);
        }
    }
    pgdir[1023] = pgdir_page | pg_entry_flags(pgdir[1023]);
    tmp_vunmap(pgdir);
    return pgdir_page;
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

static inline void kernel_stack_setup(struct task_struct *child) 
{
    uint32_t *p_stack_top = (uint32_t *)current->esp0;
    uint32_t *c_stack_top = (uint32_t *)child->esp0;

    c_stack_top[-1] = p_stack_top[-1];   // ss
    c_stack_top[-2] = p_stack_top[-2];   // esp
    c_stack_top[-3] = p_stack_top[-3];   // eflags
    c_stack_top[-4] = p_stack_top[-4];   // cs
    c_stack_top[-5] = p_stack_top[-5];   // eip

    c_stack_top[-6] = p_stack_top[-9];   // edx
    c_stack_top[-7] = p_stack_top[-10];  // ecx
    c_stack_top[-8] = 0;                   // eax 

    c_stack_top[-9] = (uint32_t)&child_return_address; // return address to trampoline

    c_stack_top[-10] = p_stack_top[-11];  // ebx
    c_stack_top[-11] = p_stack_top[-8];   // esi
    c_stack_top[-12] = p_stack_top[-7];   // edi
    c_stack_top[-13] = p_stack_top[-6];   // ebp

    child->esp = (uint32_t)&c_stack_top[-13];
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
    child->uid = child->pid; //stub
    
    child->esp0 = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!child->esp0)
        goto out_clean_pid;
    child->esp0 += KERNEL_STACK_SIZE;    
    kernel_stack_setup(child);
    
    child->vblocks.by_base = RB_ROOT;
    child->vblocks.by_size = RB_ROOT;
    if (vblocks_clone(&child->vblocks) < 0)
        goto out_clean_vblocks_and_kstack;
    
    child->mapping_files.by_base = RB_ROOT;
    if (mapping_files_clone(&child->mapping_files) < 0)
        goto out_clean_mapping_files;

    pages_set_cow();
    child->cr3 = pgdir_clone();
    
    child->time_slice_remaining = DEFAULT_TIMESLICE;
    child->parent = current;
    child->state = PROCESS_READY;

    init_list_head(&child->children);

    *out_child = child;
    return 0;

out_clean_mapping_files:
    mapping_files_clean(&child->mapping_files, false, false);
out_clean_vblocks_and_kstack:
    vblocks_clean(&child->vblocks);
    kfree((void *)(child->esp0 - KERNEL_STACK_SIZE));
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
    add_child_to_parent(current, child);
    pid_table_register(child);
    ready_queue_enqueue(child);
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
            goto out_reap_zombie;
    }
    current->state = PROCESS_WAITING_ZOMBIE;
    yield();
    // if signal
    // ret = -EINTR
    // goto out;
    list_for_each_entry(child, &current->children, child) {
        if (child->state == PROCESS_ZOMBIE)
            goto out_reap_zombie;
    }

out_reap_zombie:
    ret = reap_zombie(child, status);
out:
    return ret;
}

static inline void __attribute__((noreturn)) sys_exit(int status)
{
    user_vspace_clean(&current->vblocks, &current->mapping_files, CL_MAPPING_FREE);
    reparent_children(current);
    current->state = PROCESS_ZOMBIE;
    current->exit_status = status & 0xFF;
    // TODO: SIGCHLD
    if (current->parent->state == PROCESS_WAITING_ZOMBIE) {
        current->parent->state = PROCESS_READY;
        ready_queue_enqueue(current->parent);
    }
    yield();
    __builtin_unreachable();
}

static inline int sys_getuid(void)
{
    return current->uid;
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
    case SYS_getuid:
        ret = sys_getuid();
        break;
    }
    return ret;
}