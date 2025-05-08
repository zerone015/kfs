#ifndef _PROC_H
#define _PROC_H

#include "list.h"
#include "sched.h"

#define PID_HASH_BUCKETS        1024
#define PGROUP_HASH_BUCKETS     (PID_HASH_BUCKETS / 2)

struct pgroup {
    int pgid;                   
    struct hlist_head members;    
    struct hlist_node node;       
};

extern struct list_head process_list;
extern struct hlist_head process_table[PID_HASH_BUCKETS];
extern struct hlist_head pgroup_table[PGROUP_HASH_BUCKETS];

void proc_init(void);
void do_exit(int status) __attribute__((noreturn));

static inline void process_register(struct task_struct *proc)
{
    list_add(&proc->procl, &process_list);
    hlist_add_head(&proc->proct, &process_table[proc->pid % PID_HASH_BUCKETS]);
}

static inline void process_unregister(struct task_struct *proc)
{
    list_del(&proc->procl);
    hlist_del(&proc->proct);
}

static inline struct task_struct *process_lookup(int pid)
{
    struct hlist_head *hlist_head = &process_table[pid % PID_HASH_BUCKETS];
    struct task_struct *cur = NULL;

    hlist_for_each_entry(cur, hlist_head, proct) {
        if (cur->pid == pid)
            break;
    }
    return cur;
}

static inline void pgroup_register(struct pgroup *pgrp)
{
    hlist_add_head(&pgrp->node, &pgroup_table[pgrp->pgid % PGROUP_HASH_BUCKETS]);
}

static inline void pgroup_unregister(struct pgroup *pgrp)
{
    hlist_del(&pgrp->node);
}

static inline struct pgroup *pgroup_lookup(int pgid)
{
    struct hlist_head *hlist_head = &pgroup_table[pgid % PGROUP_HASH_BUCKETS];
    struct pgroup *cur = NULL;

    hlist_for_each_entry(cur, hlist_head, node) {
        if (cur->pgid == pgid)
            break;
    }
    return cur;
}

static inline void add_to_pgroup(struct task_struct *proc, struct pgroup *pgrp)
{
    hlist_add_head(&proc->pgroup, &pgrp->members);
}

static inline void remove_from_pgroup(struct task_struct *proc)
{
    hlist_del(&proc->pgroup);
}

static inline struct pgroup *pgroup_create(struct task_struct *leader)
{
    struct pgroup *new;

    new = kmalloc(sizeof(struct pgroup));
    if (!new)
        return NULL;
    init_hlist_head(&new->members);
    add_to_pgroup(leader, new);
    new->pgid = leader->pid;
    leader->pgid = leader->pid;
    pgroup_register(new);
    return new;
}

static inline void pgroup_destroy(struct pgroup *pgrp)
{
    pgroup_unregister(pgrp);
    kfree(pgrp);
}

static inline bool pgroup_empty(struct pgroup *pgrp)
{
    return hlist_empty(&pgrp->members);
}

static inline void add_child_to_parent(struct task_struct *child, struct task_struct *parent)
{
    list_add(&child->child, &parent->children);
}

static inline void remove_child_from_parent(struct task_struct *child)
{
    list_del(&child->child);
}

static inline void __forget_original_parent(struct task_struct *child)
{
    child->parent = process_lookup(INIT_PROCESS_PID);
    add_child_to_parent(child, child->parent);
}

static inline void forget_original_parent(struct task_struct *child)
{
    remove_child_from_parent(child);
    __forget_original_parent(child);
}

#endif