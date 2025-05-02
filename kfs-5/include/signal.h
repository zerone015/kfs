#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdbool.h>
#include "signal_types.h"
#include "sched.h"

#define	SIG_ERR	 ((sighandler_t) -1)
#define	SIG_DFL	 ((sighandler_t)  0)
#define	SIG_IGN	 ((sighandler_t)  1)

#define	SIGINT		2	
#define	SIGILL		4	
#define	SIGABRT		6	
#define	SIGFPE		8	
#define	SIGKILL		9	
#define	SIGSEGV		11	
#define	SIGTERM		15	
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP     19

#define SIG_VALID_MASK          ((1 << SIGINT) | (1 << SIGILL) | (1 << SIGABRT) | (1 << SIGFPE) \
                                | (1 << SIGKILL) | (1 << SIGSEGV) | (1 << SIGTERM) | (1 << SIGCHLD) \
                                | (1 << SIGSTOP) | (1 << SIGCONT))
#define SIG_DEFAULT_IGN_MASK    (1 << SIGCHLD)
#define SIG_CATCHABLE_MASK      (SIG_VALID_MASK & ~((1 << SIGKILL) | (1 << SIGSTOP)))

void signal_send(struct task_struct *target, int sig);

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

static inline sighandler_t signal_handler_lookup(int sig)
{
    return current->sig_handlers[sig];
}

static inline void signal_handler_register(int sig, sighandler_t handler)
{
    current->sig_handlers[sig] = handler;
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

static inline bool signal_is_registered(struct task_struct *proc, int sig)
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

static inline bool signal_pending(struct task_struct *proc)
{
    return proc->sig_pending;
}

static inline int pick_signal(struct task_struct *proc)
{
    return __builtin_clz(proc->sig_pending);
}

static inline void __signal_send(struct task_struct *target, int sig)
{
    signal_pending_set(target, sig);
    if (waiting_state(target))
        wake_up(target);
}

#endif