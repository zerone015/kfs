#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"

#define PID_MAX     32768
#define PIDMAP_MAX  (PID_MAX / 8 / 4)
#define PID_NONE    -1

enum process_state {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_STOPPED,
    PROCESS_ZOMBIE,
};

struct task_struct {
    int pid;
    int state;
    struct tast_struct *parent;
    struct list_head child;
    // memory..
    // signal queue..
    // owner..
};

extern void pid_allocator_init(void);
extern int alloc_pid(void);
extern void free_pid(int pid);

#endif