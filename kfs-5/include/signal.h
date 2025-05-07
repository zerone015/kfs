#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdbool.h>
#include "signal_types.h"
#include "sched.h"
#include "kernel.h"
#include "signal_defs.h"

int unmasked_signal_pending(void);
void signal_send(struct task_struct *target, int sig);
void do_signal(uint32_t sig_pending, struct ucontext *ucontext);

static inline void signal_init(sighandler_t *sig_handlers)
{
    for (int i = 0; i < SIG_MAX; i++)
        sig_handlers[i] = SIG_DFL;
}

static inline bool signal_is_valid(int sig)
{
    return SIG_VALID_MASK & (1 << sig);
}

static inline bool signal_is_catchable(int sig)
{
    return SIG_CATCHABLE_MASK & (1 << sig);
}

static inline sighandler_t sighandler_lookup(int sig)
{
    return current->sig_handlers[sig];
}

static inline void sighandler_register(int sig, sighandler_t handler)
{
    current->sig_handlers[sig] = handler;
}

static inline int signal_pending(struct task_struct *proc)
{
    return proc->sig_pending;
}

static inline void signal_pending_set(struct task_struct *proc, int sig)
{
    proc->sig_pending |= 1 << sig;
}

static inline void signal_pending_clear(struct task_struct *proc, int sig)
{
    proc->sig_pending &= ~(1 << sig);
}

static inline bool signal_is_default_ign(int sig)
{
    return SIG_DEFAULT_IGN_MASK & (1 << sig);
}

static inline bool signal_is_default(struct task_struct *proc, int sig)
{
    return proc->sig_handlers[sig] == SIG_DFL;
}

static inline bool signal_is_ignored(struct task_struct *proc, int sig)
{
    return proc->sig_handlers[sig] == SIG_IGN;
}

static inline bool signal_is_caught(struct task_struct *proc, int sig)
{
    return !signal_is_default(proc, sig) && !signal_is_ignored(proc, sig);
}

static inline bool signal_sendable(struct task_struct *target, int sig)
{
    if (signal_is_ignored(target, sig))
        return false;
    if (signal_is_default_ign(sig) && signal_is_default(target, sig))
        return false;
    return true;
}

static inline int pick_signal(uint32_t sig_pending)
{
    return __builtin_ctz(sig_pending);
}

static inline bool current_signal(struct task_struct *proc, int sig)
{
    return proc->current_signal == sig;
}

static inline void current_signal_set(struct task_struct *proc, int sig)
{
    proc->current_signal = sig;
}

static inline void current_signal_clear(struct task_struct *proc)
{
    proc->current_signal = 0;
}

static inline bool current_signal_empty(struct task_struct *proc)
{
    return !proc->current_signal;
}

static inline void __signal_send(struct task_struct *target, int sig)
{
    signal_pending_set(target, sig);
    if (target->current_signal != sig)
        wake_up(target);
}

static inline void caught_signal_send(struct task_struct *target, int sig)
{
    if (signal_is_caught(target, sig))
        __signal_send(target, sig);
}

#endif