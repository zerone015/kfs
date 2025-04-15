#ifndef _PROC_H
#define _PROC_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "hmm.h"
#include "sched_types.h"
#include "interrupt.h"

extern uint16_t *page_ref;

#define INIT_PROCESS_PID        0
#define KERNEL_STACK_SIZE       (PAGE_SIZE * 2)

void proc_init(void);
void __attribute__((noreturn)) init_process(void);
int fork(struct interrupt_frame *iframe);
void __attribute__((noreturn)) exit(int status);

static inline void kthread_stack_free(struct task_struct *task)
{
    kfree((void *)(task->esp0 - KERNEL_STACK_SIZE));
}

#endif