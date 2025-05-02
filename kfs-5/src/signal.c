#include "signal.h"
#include "proc.h"

static void sigstop_handle(struct task_struct *target)
{
    signal_pending_clear(target, SIGCONT);
    __signal_send(target, SIGSTOP);
}

static void sigcont_handle(struct task_struct *target, int sig)
{
    signal_pending_clear(target, SIGSTOP);
    if (target->state == PROCESS_STOPPED) {
        wake_up(target);
        if (signal_is_registered(target->parent, SIGCHLD))
            __signal_send(target->parent, SIGCHLD);
    }
    if (signal_is_registered(target, sig))
        signal_pending_set(target, sig);
}

static void sigkill_handle(struct task_struct *target)
{
    signal_pending_set(target, SIGKILL);
    if (non_runnable_state(target))
        wake_up(target);
}

void signal_send(struct task_struct *target, int sig)
{
    switch (sig) {
    case SIGSTOP:
        sigstop_handle(target);
        break;
    case SIGCONT:
        sigcont_handle(target, sig);
        break;
    case SIGKILL:
        sigkill_handle(target);
        break;
    default:
        if (signal_sendable(target, sig))
            __signal_send(target, sig);
    }
}