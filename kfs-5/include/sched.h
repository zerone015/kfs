#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"
#include "interrupt.h"
#include "gdt.h"
#include "pid.h"
#include "printk.h"
#include "signal_types.h"
#include "sched_defs.h"
#include <stdbool.h>

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