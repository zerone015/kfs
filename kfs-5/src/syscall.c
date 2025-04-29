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
#include "proc.h"

static inline void add_child_to_parent(struct task_struct *parent, struct task_struct *child)
{
    list_add(&child->child, &parent->children);
}

static inline void remove_child_from_parent(struct task_struct *child)
{
    list_del(&child->child);
}

static inline bool has_kill_permission(struct task_struct *target)
{
    if (current->uid == 0 || current->euid == 0)
        return true;
    if (current->uid == target->uid || current->uid == target->suid \
        || current->euid == target->uid || current->euid == target->suid)
        return true;
    return false;
}

static inline void signal_send(struct task_struct *target, int sig)
{
    if (sig != 0)
        sig_pending_set(target, sig);
}


static inline int kill_to_group(struct pgroup *pg, int sig)
{
    struct task_struct *target;
    bool sent = false;

    hlist_for_each_entry(target, &pg->members, pgroup) {
        if (has_kill_permission(target)) {
            signal_send(target, sig);
            sent = true;
        }
    }
    return sent ? 0 : -EPERM;
}

static inline int kill_to_all(int sig)
{
    struct task_struct *target;
    bool sent = false;

    list_for_each_entry(target, &process_list, procl) {
        if (target->pid != INIT_PROCESS_PID && has_kill_permission(target)) {
            signal_send(target, sig);
            sent = true;
        }
    }
    return sent ? 0 : -EPERM;
}

static inline int kill_one(int pid, int sig)
{
    struct task_struct *target;

    if (pid >= PID_MAX)
        return -ESRCH;
    target = process_lookup(pid);
    if (!target)
        return -ESRCH;
    if (!has_kill_permission(target))
        return -EPERM;
    signal_send(target, sig);
    return 0;
}

static inline int kill_many(int pid, int sig)
{
    struct pgroup *pg;

    if (pid == -1)
        return kill_to_all(sig);
    if (pid < -1) {
        if (-pid >= PGID_MAX || !(pg = pgroup_lookup(-pid)))
            return -ESRCH;
    }
    else {
        pg = pgroup_lookup(current->pgid);
    }
    return kill_to_group(pg, sig);
}

static inline void forget_original_parent(struct task_struct *proc)
{
    proc->parent = process_lookup(INIT_PROCESS_PID);
    add_child_to_parent(proc->parent, proc);
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
    process_unregister(child);
    
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

static inline int process_clone(struct task_struct **out_new)
{
    struct task_struct *new;
    int ret = -ENOMEM;

    new = kmalloc(sizeof(struct task_struct));
    if (!new)
        goto out;

    new->pid = alloc_pid();
    if (new->pid == PID_NONE) {
        ret = -EAGAIN;
        goto out_clean_new;
    }
    new->uid = current->uid;
    new->euid = current->euid;
    new->suid = current->suid;
    new->pgid = current->pgid;
    new->sid = current->sid;
    
    new->esp0 = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!new->esp0)
        goto out_clean_pid;
    new->esp0 += KERNEL_STACK_SIZE;    
    kernel_stack_setup(new);
    
    new->vblocks.by_base = RB_ROOT;
    new->vblocks.by_size = RB_ROOT;
    if (vblocks_clone(&new->vblocks) < 0)
        goto out_clean_vblocks_and_kstack;
    
    new->mapping_files.by_base = RB_ROOT;
    if (mapping_files_clone(&new->mapping_files) < 0)
        goto out_clean_mapping_files;

    pages_set_cow();
    new->cr3 = pgdir_clone();

    memcpy32(new->sig_handlers, current->sig_handlers, sizeof(current->sig_handlers) / 4);
    new->sig_pending = 0;
    
    new->time_slice_remaining = DEFAULT_TIMESLICE;
    new->parent = current;
    new->state = PROCESS_READY;

    init_list_head(&new->children);

    *out_new = new;
    return 0;

out_clean_mapping_files:
    mapping_files_clean(&new->mapping_files, false, false);
out_clean_vblocks_and_kstack:
    vblocks_clean(&new->vblocks);
    kfree((void *)(new->esp0 - KERNEL_STACK_SIZE));
out_clean_pid:
    free_pid(new->pid);
out_clean_new:
    kfree(new);
out:
    return ret;
}

static inline int sys_fork(void)
{
    struct task_struct *new;
    int ret;

    ret = process_clone(&new);
    if (ret < 0)
        return ret;
    add_child_to_parent(current, new);
    process_register(new);
    ready_queue_enqueue(new);
    return new->pid;
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
    process_unregister(current);
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

static inline int sys_getpid(void)
{
    return current->pid;
}

static inline int sys_getuid(void)
{
    return current->uid;
}

static inline int sys_signal(int sig, uintptr_t handler)
{
    sighandler_t old;

    if (!signal_is_valid(sig) || !signal_is_catchable(sig))
        return -EINVAL;
    old = sig_handler_lookup(sig);
    sig_handler_register(sig, (sighandler_t)handler);
    return (int)old;
}

static inline int sys_kill(int pid, int sig)
{
    if (sig != 0 && !signal_is_valid(sig))
        return -EINVAL;
    if (pid > 0)
        return kill_one(pid, sig);
    return kill_many(pid, sig);
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
    case SYS_getpid:
        ret = sys_getpid();
        break;
    case SYS_getuid:
        ret = sys_getuid();
        break;
    case SYS_kill:
        ret = sys_kill(sframe.arg1, sframe.arg2);
        break;
    case SYS_signal:
        ret = sys_signal(sframe.arg1, sframe.arg2);
        break;
    }
    return ret;
}