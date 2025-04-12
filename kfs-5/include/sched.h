#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"
#include "interrupt.h"
#include "gdt.h"

#define DEFAULT_TIMESLICE 10

extern struct task_struct *current;
extern struct list_head ready_queue;

enum process_state {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_STOPPED,
    PROCESS_ZOMBIE,
};

struct cpu_context {
    size_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
    size_t eflags, eip;
};

struct task_struct {
    int pid;
    int state;
    size_t time_slice_remaining;
    size_t cr3;
    struct cpu_context cpu_context;
    struct task_struct *parent; 
    struct list_head child_list;
    struct list_head child;
    struct list_head ready;
    struct user_vblock_tree vblocks;
    struct mapping_file_tree mapping_files;
    // memory..
    // signal queue..
    // owner..
};

void scheduler_init(void);

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

static inline void task_run(struct task_struct *task)
{
    struct cpu_context *ctx;

    task->state = PROCESS_RUNNING;
    task->time_slice_remaining = DEFAULT_TIMESLICE;

    asm volatile("mov %0, %%cr3" :: "r"(task->cr3) : "memory");

    asm volatile(
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        :
        : "a"(GDT_SELECTOR_DATA_PL3)
        : "memory"
    );

    ctx = &task->cpu_context;
    asm volatile (
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
          "i"(GDT_SELECTOR_DATA_PL3), "m"(ctx->esp), "m"(ctx->eflags),
          "i"(GDT_SELECTOR_CODE_PL3), "m"(ctx->eip),
          "r"((uint16_t)GDT_SELECTOR_DATA_PL3)
        : "memory"
    );
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

static inline void schedule(struct interrupt_frame *iframe)
{
    cpu_context_save(iframe);
    ready_queue_enqueue(current);
    current = ready_queue_dequeue();
    task_run(current);
}

#endif