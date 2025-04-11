#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"
#include "interrupt.h"
#include "gdt.h"

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

struct process_context {
    size_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
    size_t eflags, eip;
    size_t cr3;
};

struct task_struct {
    int pid;
    int state;
    size_t time_slice_remaining;
    struct task_struct *parent; 
    struct list_head child_list;
    struct list_head child;
    struct list_head ready;
    struct process_context context;
    struct user_vblock_tree vblocks;
    struct mapping_file_tree mapping_files;
    // memory..
    // signal queue..
    // owner..
};

void scheduler_init(void);

static inline void __context_save(struct interrupt_frame *iframe)
{
    current->context.eax = iframe->eax;
    current->context.ebp = iframe->ebp;
    current->context.ebx = iframe->ebx;
    current->context.ecx = iframe->ecx;
    current->context.edi = iframe->edi;
    current->context.edx = iframe->edx;
    current->context.esi = iframe->esi;
    current->context.eflags = iframe->eflags;
    current->context.eip = iframe->eip;
    current->context.esp = iframe->esp;
    current->state = PROCESS_READY;
}

static inline void __next_task_run(void)
{
    struct process_context *ctx;

    current->state = PROCESS_RUNNING;
    ctx = &current->context;
    asm volatile("mov %0, %%cr3" :: "r"(ctx->cr3) : "memory");
    asm volatile(
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        :
        : "a"(GDT_SELECTOR_DATA_PL3)
        : "memory"
    );
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
        "iret\n\t"
        :
        : "m"(ctx->eax), "m"(ctx->ebx), "m"(ctx->ecx), "m"(ctx->edx),
          "m"(ctx->esi), "m"(ctx->edi), "m"(ctx->ebp),
          "i"(GDT_SELECTOR_DATA_PL3), "m"(ctx->esp), "m"(ctx->eflags),
          "i"(GDT_SELECTOR_CODE_PL3), "m"(ctx->eip)
        : "memory"
    );
}

static inline void task_enqueue(struct task_struct *ts)
{
    list_add_tail(&ts->ready, &ready_queue);
}

static inline struct task_struct *task_dequeue(void)
{
    struct task_struct *ts;

    ts = list_first_entry(&ready_queue, struct task_struct, ready);
    list_del(&ts->ready);
    return ts;
}

static inline void schedule(struct interrupt_frame *iframe)
{
    __context_save(iframe);
    task_enqueue(current);
    current = task_dequeue();
    __next_task_run();
}

#endif