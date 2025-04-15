#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"
#include "interrupt.h"
#include "gdt.h"
#include "pid.h"
#include "sched_types.h"
#include "offsets.h"

#define DEFAULT_TIMESLICE   10
#define PID_TABLE_MAX       PID_MAX

extern struct task_struct *current;
extern struct list_head ready_queue;
extern struct task_struct *pid_table[PID_TABLE_MAX];

void scheduler_init(void);
void __attribute__((naked, noreturn)) yield(void);

static inline void cpu_context_save(struct interrupt_frame *iframe)
{
    current->cpu_context.eax = iframe->eax;
    current->cpu_context.ebp = iframe->ebp;
    current->cpu_context.ebx = iframe->ebx;
    current->cpu_context.ecx = iframe->ecx;
    current->cpu_context.edi = iframe->edi;
    current->cpu_context.edx = iframe->edx;
    current->cpu_context.esi = iframe->esi;
    current->cpu_context.eflags = iframe->eflags;
    current->cpu_context.eip = iframe->eip;
    current->cpu_context.esp = iframe->esp;
    current->state = PROCESS_READY;
}

static inline __attribute__((noreturn)) void task_run(struct task_struct *task)
{
    struct cpu_context *ctx;
    uint32_t cs, ds;

    tss_esp0_update(task->esp0);

    task->state = PROCESS_RUNNING;
    task->time_slice_remaining = DEFAULT_TIMESLICE;

    __asm__ volatile("mov %0, %%cr3" :: "r"(task->cr3) : "memory");

    ctx = &task->cpu_context;

    if (ctx->eip >= K_VSPACE_START) {
        cs = GDT_SELECTOR_CODE_PL0;
        ds = GDT_SELECTOR_DATA_PL0;
    } else {
        cs = GDT_SELECTOR_CODE_PL3;
        ds = GDT_SELECTOR_DATA_PL3;
    }

    __asm__ volatile (
        "movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "movl %3, %%edx\n\t"
        "movl %4, %%esi\n\t"
        "movl %5, %%edi\n\t"
        "movl %6, %%ebp\n\t"
        "pushl %7\n\t"         
        "pushl %8\n\t"         
        "pushl %9\n\t"         
        "pushl %10\n\t"        
        "pushl %11\n\t"        
        "movw %12, %%ds\n\t"
        "movw %12, %%es\n\t"
        "iret\n\t"
        :
        : "m"(ctx->eax), "m"(ctx->ebx), "m"(ctx->ecx), "m"(ctx->edx),
          "m"(ctx->esi), "m"(ctx->edi), "m"(ctx->ebp),
          "r"(ds), "m"(ctx->esp), "m"(ctx->eflags),
          "r"(cs), "m"(ctx->eip), "r"((uint16_t)ds)
        : "memory"
    );
    __builtin_unreachable();
}

static inline void ready_queue_enqueue(struct task_struct *task)
{
    list_add_tail(&task->ready, &ready_queue);
}

static inline struct task_struct *ready_queue_dequeue(void)
{
    struct task_struct *task;

    task = list_first_entry(&ready_queue, struct task_struct, ready);
    list_del(&task->ready);
    return task;
}

static inline __attribute__((noreturn)) void schedule(struct interrupt_frame *iframe)
{
    cpu_context_save(iframe);
    ready_queue_enqueue(current);
    current = ready_queue_dequeue();
    task_run(current);
}

#endif