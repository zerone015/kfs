#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"
#include "interrupt.h"
#include "gdt.h"
#include "pid.h"

#define DEFAULT_TIMESLICE   10
#define PID_TABLE_MAX       PID_MAX

enum process_state {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_STOPPED,
    PROCESS_ZOMBIE,
    PROCESS_WAITING_ZOMBIE,
};

struct task_struct {
    int pid;
    uint32_t cr3;
    uint32_t esp;
    uint32_t esp0;
    size_t time_slice_remaining;
    int exit_status;
    struct task_struct *parent; 
    struct list_head children;
    struct list_head child;
    struct list_head ready;
    struct user_vblock_tree vblocks;
    struct mapping_file_tree mapping_files;
    uint8_t state;
    // memory..
    // signal queue..
    // owner..
};

extern struct task_struct *current;
extern struct task_struct *pid_table[PID_TABLE_MAX];

void switch_to_task(struct task_struct *next_task);

static inline void pid_table_register(struct task_struct *task)
{
    pid_table[task->pid] = task;
}

static inline void pid_table_unregister(int pid)
{
    pid_table[pid] = NULL;
}

static inline struct task_struct *pid_table_lookup(int pid)
{
    return pid_table[pid];
}

static inline void add_child_to_parent(struct task_struct *parent, struct task_struct *child)
{
    list_add(&child->child, &parent->children);
}

static inline void remove_child_from_parent(struct task_struct *child)
{
    list_del(&child->child);
}

static inline void ready_queue_register(struct task_struct *task)
{
    list_add_tail(&task->ready, &current->ready);
}

static inline void ready_queue_unregister(struct task_struct *task)
{
    list_del(&task->ready);
}

static inline void schedule(void)
{
    struct task_struct *next_task;

    next_task = list_next_entry(current, ready);

    current->state = PROCESS_READY;
    next_task->state = PROCESS_RUNNING;

    switch_to_task(next_task);
};

static inline void yield(void)
{
    struct task_struct *next_task;

    next_task = list_next_entry(current, ready);
    
    current->state = PROCESS_WAITING;
    next_task->state = PROCESS_RUNNING;
    
    ready_queue_unregister(current);
    
    switch_to_task(next_task);
};

#endif