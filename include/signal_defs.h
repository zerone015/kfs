#ifndef _SIGNAL_DEFS_H
#define _SIGNAL_DEFS_H

#define SIG_MAX     20

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

struct signal_frame {
    uint32_t ret;
    uint32_t arg;
    uint8_t trampoline[8];
    uint32_t esp, ebp, edi, esi, ebx, edx, ecx, eax;
    uint32_t eip, eflags;
};

typedef void (*sighandler_t)(int);

#endif