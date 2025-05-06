#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"
#include "interrupt.h"
#include "gdt.h"
#include "pid.h"
#include "printk.h"
#include "signal_types.h"
#include <stdbool.h>

#define DEFAULT_TIMESLICE       10
#define WAITING_STATE_MASK      ((1 << PROCESS_WAIT_CHILD_PID) | (1 << PROCESS_WAIT_CHILD_PGID) | \
                                (1 << PROCESS_WAIT_CHILD_ANY))
#define SLEEP_STATE_MASK        (WAITING_STATE_MASK | (1 << PROCESS_STOPPED))

enum process_state {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_STOPPED,
    PROCESS_ZOMBIE,
    PROCESS_WAIT_CHILD_PID,
    PROCESS_WAIT_CHILD_PGID,
    PROCESS_WAIT_CHILD_ANY,
};

struct task_struct {
    int pid;
    int uid;
    int pgid;
    int euid;
    int suid;
    int sid;
    uint32_t cr3;
    uint32_t esp;
    uint32_t esp0;
    size_t time_slice_remaining;
    int status;
    struct task_struct *parent; 
    struct list_head children;
    struct list_head child;
    struct list_head ready;
    struct list_head procl;
    struct hlist_node proct;
    struct hlist_node pgroup;
    struct user_vblock_tree vblocks;
    struct mapping_file_tree mapping_files;
    uint32_t sig_pending;
    sighandler_t sig_handlers[SIG_MAX];
    int wait_id;
    uint8_t state;
    uint8_t current_signal;
};

extern struct task_struct *current;

void switch_to_task(struct task_struct *next_task);

static inline void ready_queue_enqueue(struct task_struct *task)
{
    list_add_tail(&task->ready, &current->ready);
}

static inline void ready_queue_dequeue(struct task_struct *task)
{
    list_del(&task->ready);
}

static inline void schedule(void)
{
    struct task_struct *next_task;

    next_task = list_next_entry(current, ready);
    next_task->time_slice_remaining = DEFAULT_TIMESLICE;
    switch_to_task(next_task);
};

static inline void yield(void)
{
    ready_queue_dequeue(current);
    schedule();
};

static inline void sleep(struct task_struct *target, uint8_t state)
{
    target->state = state;
    yield();
}

static inline bool waiting_state(struct task_struct *target)
{
    return WAITING_STATE_MASK & (1 << target->state);
}

static inline bool runnable_state(struct task_struct *target)
{
    return !(SLEEP_STATE_MASK & (1 << target->state));
}

static inline void __wake_up(struct task_struct *target)
{
    target->state = PROCESS_READY;
    ready_queue_enqueue(target);
}

static inline void wake_up(struct task_struct *target)
{
    if (waiting_state(target))
        __wake_up(target);
}

#endif