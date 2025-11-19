#ifndef _SCHED_DEFS_H
#define _SCHED_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include "list.h"
#include "rbtree.h"
#include "vmm_defs.h"
#include "signal_defs.h"

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
    struct u_vblock_tree vblocks;
    struct mapped_vblock_tree mapped_vblocks;
    uint32_t sig_pending;
    sighandler_t sig_handlers[SIG_MAX];
    int wait_id;
    uint8_t state;
    uint8_t current_signal;
};


#endif