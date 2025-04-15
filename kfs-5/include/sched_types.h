#ifndef _SCHED_TYPES_H
#define _SCHED_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include "list.h"
#include "vmm_types.h"

enum process_state {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_STOPPED,
    PROCESS_ZOMBIE,
    PROCESS_WAITING_EXIT_CHILD,
};

struct cpu_context {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
    uint32_t eflags, eip;
};

struct task_struct {
    int pid;
    int state;
    size_t time_slice_remaining;
    uint32_t cr3;
    uint32_t esp0;
    int exit_status;
    struct cpu_context cpu_context;
    struct task_struct *parent; 
    struct list_head children;
    struct list_head child;
    struct list_head ready;
    struct user_vblock_tree vblocks;
    struct mapping_file_tree mapping_files;
    // memory..
    // signal queue..
    // owner..
};

#endif