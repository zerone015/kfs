#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdbool.h>
#include "signal_types.h"
#include "sched.h"

#define	SIG_ERR	 ((sighandler_t) -1)
#define	SIG_DFL	 ((sighandler_t)  0)
#define	SIG_IGN	 ((sighandler_t)  1)

#define	SIGINT		2	/* Interactive attention signal.  */
#define	SIGILL		4	/* Illegal instruction.  */
#define	SIGABRT		6	/* Abnormal termination.  */
#define	SIGFPE		8	/* Erroneous arithmetic operation.  */
#define	SIGKILL		9	/* Killed.  */
#define	SIGSEGV		11	/* Invalid access to storage.  */
#define	SIGTERM		15	/* Termination request.  */
#define SIGCHLD		17

#define SIG_VALID_MASK  ((1 << SIGINT) | (1 << SIGILL) | (1 << SIGABRT) | (1 << SIGFPE) \
                    | (1 << SIGKILL) | (1 << SIGSEGV) | (1 << SIGTERM) | (1 << SIGCHLD))

static inline void sig_handlers_init(sighandler_t *sig_handlers)
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
    return sig != SIGKILL;
}

static inline sighandler_t sig_handler_lookup(int sig)
{
    return current->sig_handlers[sig];
}

static inline void sig_handler_register(int sig, sighandler_t handler)
{
    current->sig_handlers[sig] = handler;
}

static inline void sig_pending_set(struct task_struct *target, int sig)
{
    target->sig_pending |= 1 << sig;
}

static inline void sig_pending_clear(struct task_struct *target, int sig)
{
    target->sig_pending &= ~(1 << sig);
}

static inline void signal_send(struct task_struct *target, int sig)
{
    if (sig != 0)
        sig_pending_set(target, sig);
}

#endif