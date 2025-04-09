#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "vmm.h"

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
    size_t eflags, cs, eip;
    size_t ss, ds, gs, fs;
    size_t cr3;
};

struct task_struct {
    int pid;
    int state;
    struct task_struct *parent;
    struct list_head child;
    struct list_head ready;
    struct process_context context;
    struct user_vblock_tree vblocks;
    struct mapping_file_tree mapping_files;
    // memory..
    // signal queue..
    // owner..
};

extern struct task_struct *current;

extern void scheduler_init(void);
extern void schedule_process(struct task_struct *ts);

#endif