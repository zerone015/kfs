#include "proc.h"
#include "pid.h"
#include "signal.h"
#include "sched.h"

struct list_head process_list;
struct hlist_head process_table[PID_HASH_BUCKETS];
struct hlist_head pgroup_table[PGROUP_HASH_BUCKETS];

void proc_init(void)
{
    pidmap_init();
    init_list_head(&process_list);
}

static void defunct_mark(int status)
{
    current->status = status;
    current->state = PROCESS_ZOMBIE;
}

static void parent_wake_up(int wait_id)
{
    struct task_struct *parent = current->parent;

    switch (parent->state) {
    case PROCESS_WAIT_CHILD_ANY:
        if (wait_id != -1 || list_empty(&parent->children)) {
            parent->wait_id = wait_id;
            __wake_up(parent);
        }
        break;
    case PROCESS_WAIT_CHILD_PID:
        if (parent->wait_id == current->pid) {
            parent->wait_id = wait_id;
            __wake_up(parent);
        }
        break;
    case PROCESS_WAIT_CHILD_PGID:
        if (parent->wait_id == current->pgid) {
            parent->wait_id = wait_id;
            __wake_up(parent);
        }
        break;
    }
}

static void exit_notify(struct task_struct *parent)
{
    if (signal_is_ignored(parent, SIGCHLD)) {
        forget_original_parent(current);
        parent_wake_up(-1);
    } else if (!signal_is_default(parent, SIGCHLD)) {
        __signal_send(parent, SIGCHLD);
    } else {
        parent_wake_up(current->pid);
    }
}

static void reparent_children(void)
{
    struct task_struct *child;

    list_for_each_entry(child, &current->children, child) {
        __forget_original_parent(child);
    }
}

static void resources_cleanup(void) 
{
    struct pgroup *pgrp;

    user_vas_cleanup(&current->vblocks, &current->mapped_vblocks, CL_MAPPING_FREE);

    remove_from_pgroup(current);
    
    pgrp = pgroup_lookup(current->pgid);
    if (pgroup_empty(pgrp))
        pgroup_destroy(pgrp);
}

void do_exit(int status)
{
    resources_cleanup();
    reparent_children();
    defunct_mark(status);
    exit_notify(current->parent);
    yield();
    __builtin_unreachable();
}