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
#include "kernel.h"

static bool check_kill_permission(struct task_struct *target)
{
    if (current->uid == 0 || current->euid == 0)
        return true;
    if (current->uid == target->uid || current->uid == target->suid \
        || current->euid == target->uid || current->euid == target->suid)
        return true;
    return false;
}

static int kill_to_group(struct pgroup *pgrp, int sig)
{
    struct task_struct *target;
    bool sent = false;

    hlist_for_each_entry(target, &pgrp->members, pgroup) {
        if (check_kill_permission(target)) {
            if (sig != 0)
                signal_send(target, sig);
            sent = true;
        }
    }
    return sent ? 0 : -EPERM;
}

static int kill_to_all(int sig)
{
    struct task_struct *target;
    bool sent = false;

    list_for_each_entry(target, &process_list, procl) {
        if (target->pid != INIT_PROCESS_PID && check_kill_permission(target)) {
            if (sig != 0)
                signal_send(target, sig);
            sent = true;
        }
    }
    return sent ? 0 : -EPERM;
}

static int kill_one(int pid, int sig)
{
    struct task_struct *target;

    if (pid >= PID_MAX)
        return -ESRCH;
    target = process_lookup(pid);
    if (!target)
        return -ESRCH;
    if (!check_kill_permission(target))
        return -EPERM;
    if (sig != 0)
        signal_send(target, sig);
    return 0;
}

static int kill_many(int pid, int sig)
{
    struct pgroup *pgrp;

    if (pid == -1)
        return kill_to_all(sig);
    if (pid < -1) {
        if (-pid >= PGID_MAX || !(pgrp = pgroup_lookup(-pid)))
            return -ESRCH;
    }
    else {
        pgrp = pgroup_lookup(current->pgid);
    }
    return kill_to_group(pgrp, sig);
}

static struct task_struct *find_zombie_pgid(struct task_struct *parent, int pgid, bool *found)
{
    struct task_struct *child;

    *found = false;
    list_for_each_entry(child, &parent->children, child) {
        if (child->pgid == pgid) {
            *found = true;
            if (child->state == PROCESS_ZOMBIE)
                return child;
        }
    }
    return NULL;
}

static struct task_struct *find_zombie(struct task_struct *parent)
{
    struct task_struct *child;

    list_for_each_entry(child, &parent->children, child) {
        if (child->state == PROCESS_ZOMBIE)
            return child;
    }
    return NULL;
}

static int reap_zombie(struct task_struct *child, int *status)
{
    int ret;
    
    if (status) {
        if (!user_memory_writable(status))
            return -EFAULT;
        *status = child->status;
    }
    if (current_signal(current, SIGCHLD)) {
        if (!find_zombie(current))
            signal_pending_clear(current, SIGCHLD);
    }
    free_pid(child->pid);
    free_pages(child->cr3, PAGE_SIZE);
    kfree((void *)(child->esp0 - KERNEL_STACK_SIZE));
    remove_child_from_parent(child);
    process_unregister(child);
    ret = child->pid;
    kfree(child);
    return ret;
}

static int wait_for_child(uint8_t state, int *status, int options)
{
    struct task_struct *child;

    if (options == WNOHANG)
        return 0;
    if (unmasked_signal_pending())
        return -EINTR;
    sleep(current, state);
    if (signal_pending(current))
        return -EINTR;
    if (current->wait_id == -1)
        return -ECHILD;
    child = process_lookup(current->wait_id);
    return reap_zombie(child, status);
}

static int wait_for_pid(int pid, int *status, int options)
{
    struct task_struct *child;

    child = process_lookup(pid);
    if (!child || current != child->parent)
        return -ECHILD;
    if (child->state == PROCESS_ZOMBIE)
        return reap_zombie(child, status);
    current->wait_id = child->pid;
    return wait_for_child(PROCESS_WAIT_CHILD_PID, status, options);
}

static int wait_for_pgid(int pgid, int *status, int options)
{
    struct task_struct *child;
    bool found;
    
    child = find_zombie_pgid(current, pgid, &found);
    if (child)
        return reap_zombie(child, status);
    if (!found)
        return -ECHILD;
    current->wait_id = pgid;
    return wait_for_child(PROCESS_WAIT_CHILD_PGID, status, options);
}

static int wait_for_any(int *status, int options)
{
    struct task_struct *child;

    if (list_empty(&current->children))
        return -ECHILD;
    child = find_zombie(current);
    if (child)
        return reap_zombie(child, status);
    return wait_for_child(PROCESS_WAIT_CHILD_ANY, status, options);
}

static void pages_set_cow(void) 
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

static page_t pgdir_clone(void)
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

static int vblocks_clone(struct user_vblock_tree *vblocks)
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

static int mapping_files_clone(struct mapping_file_tree *mapping_files)
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

static void child_stack_setup(struct task_struct *child, struct ucontext *pcontext) 
{
    uint32_t *c_stack_top = (uint32_t *)child->esp0;

    c_stack_top[-1] = pcontext->ss;         // ss
    c_stack_top[-2] = pcontext->esp;        // esp
    c_stack_top[-3] = pcontext->eflags;     // eflags
    c_stack_top[-4] = pcontext->cs;         // cs
    c_stack_top[-5] = pcontext->eip;        // eip

    c_stack_top[-6] = 0;                    // eax 
    c_stack_top[-7] = pcontext->ecx;        // ecx
    c_stack_top[-8] = pcontext->edx;        // edx

    c_stack_top[-9] = (uint32_t)&fork_child_trampoline; // return address to trampoline

    c_stack_top[-10] = pcontext->ebx;       // ebx
    c_stack_top[-11] = pcontext->esi;       // esi
    c_stack_top[-12] = pcontext->edi;       // edi
    c_stack_top[-13] = pcontext->ebp;       // ebp

    child->esp = (uint32_t)&c_stack_top[-13];
}

static int process_clone(struct ucontext *ucontext, struct task_struct **out_child)
{
    struct task_struct *child;
    int ret = -ENOMEM;

    child = kmalloc(sizeof(struct task_struct));
    if (!child)
        goto out;

    child->pid = alloc_pid();
    if (child->pid == PID_NONE) {
        ret = -EAGAIN;
        goto out_clean_new;
    }
    child->uid = current->uid;
    child->euid = current->euid;
    child->suid = current->suid;
    child->pgid = current->pgid;
    child->sid = current->sid;
    
    child->esp0 = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!child->esp0)
        goto out_clean_pid;
    child->esp0 += KERNEL_STACK_SIZE;    
    child_stack_setup(child, ucontext);
    
    child->vblocks.by_base = RB_ROOT;
    child->vblocks.by_size = RB_ROOT;
    if (vblocks_clone(&child->vblocks) < 0)
        goto out_clean_vblocks_and_kstack;
    
    child->mapping_files.by_base = RB_ROOT;
    if (mapping_files_clone(&child->mapping_files) < 0)
        goto out_clean_mapping_files;

    pages_set_cow();
    child->cr3 = pgdir_clone();

    memcpy32(child->sig_handlers, current->sig_handlers, sizeof(current->sig_handlers) / 4);
    child->sig_pending = 0;
    child->current_signal = 0;
    
    child->time_slice_remaining = DEFAULT_TIMESLICE;
    child->parent = current;
    child->state = PROCESS_READY;

    init_list_head(&child->children);

    *out_child = child;
    return 0;

out_clean_mapping_files:
    mapping_files_cleanup(&child->mapping_files, false, false);
out_clean_vblocks_and_kstack:
    vblocks_cleanup(&child->vblocks);
    kfree((void *)(child->esp0 - KERNEL_STACK_SIZE));
out_clean_pid:
    free_pid(child->pid);
out_clean_new:
    kfree(child);
out:
    return ret;
}

static int sys_fork(struct ucontext *ucontext)
{
    struct task_struct *child;
    struct pgroup *pgrp;
    int ret;

    ret = process_clone(ucontext, &child);
    if (ret < 0)
        return ret;

    add_child_to_parent(child, current);
    process_register(child);

    pgrp = pgroup_lookup(child->pgid);
    add_to_pgroup(child, pgrp);

    ready_queue_enqueue(child);
    return child->pid;
}

static int sys_waitpid(int pid, int *status, int options)
{
    if (options != 0 && options != WNOHANG)
        return -EINVAL;
    if (pid > 0)
        return wait_for_pid(pid, status, options);
    if (pid == 0)
        return wait_for_pgid(current->pgid, status, options);
    if (pid < -1)
        return pid == INT_MIN ? -ESRCH : wait_for_pgid(-pid, status, options);
    return wait_for_any(status, options);
}

static void __attribute__((noreturn)) sys_exit(int status) 
{
    do_exit((status & 0xFF) << 8);
    __builtin_unreachable();
}

static int sys_getpid(void)
{
    return current->pid;
}

static int sys_getuid(void)
{
    return current->uid;
}

static int sys_signal(int sig, uintptr_t handler)
{
    sighandler_t old;

    if (!signal_is_valid(sig) || !signal_is_catchable(sig))
        return -EINVAL;
    old = sighandler_lookup(sig);
    sighandler_register(sig, (sighandler_t)handler);
    return (int)old;
}

static int sys_kill(int pid, int sig)
{
    if (sig != 0 && !signal_is_valid(sig))
        return -EINVAL;
    if (pid > 0)
        return kill_one(pid, sig);
    return kill_many(pid, sig);
}

static int sys_sigreturn(struct ucontext *ucontext)
{
    struct signal_frame *sf = container_of(ucontext->esp, struct signal_frame, arg);
    
    if (kernel_space(sf->eip) || (int)sf->eax == -EINTR)
        do_exit(SIGSEGV);

    ucontext->ecx = sf->ecx;
    ucontext->edx = sf->edx;
    ucontext->ebx = sf->ebx;
    ucontext->esi = sf->esi;
    ucontext->edi = sf->edi;
    ucontext->ebp = sf->ebp;
    ucontext->esp = sf->esp;
    ucontext->eip = sf->eip;
    ucontext->eflags = (ucontext->eflags & ~EFLAGS_USER_MASK) | (sf->eflags & EFLAGS_USER_MASK);

    current_signal_clear(current);

    return sf->eax;
}

int syscall_dispatch(struct ucontext *ucontext)
{
    int ret = -ENOSYS;

    switch (ucontext->eax) {
    case SYS_exit:
        sys_exit(ucontext->ebx);
    case SYS_fork:
        ret = sys_fork(ucontext);
        break;
    case SYS_write:
        printk((const char *)ucontext->ebx);
        ret = 0;
        break;
    case SYS_waitpid:
        ret = sys_waitpid(ucontext->ebx, (int *)ucontext->ecx, ucontext->edx);
        break;
    case SYS_getpid:
        ret = sys_getpid();
        break;
    case SYS_getuid:
        ret = sys_getuid();
        break;
    case SYS_kill:
        ret = sys_kill(ucontext->ebx, ucontext->ecx);
        break;
    case SYS_signal:
        ret = sys_signal(ucontext->ebx, ucontext->ecx);
        break;
    case SYS_sigreturn:
        ret = sys_sigreturn(ucontext);
        break;
    }
    return ret;
}