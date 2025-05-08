#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <stdint.h>
#include "pmm.h"
#include "sched.h"
#include "signal.h"
#include "kernel.h"

#define SYS_exit        1
#define SYS_fork        2
#define SYS_read        3
#define SYS_write       4
#define SYS_open        5
#define SYS_close       6
#define SYS_waitpid     7       
#define SYS_execve      11
#define SYS_getpid      20
#define SYS_getuid      24
#define SYS_kill        37
#define SYS_signal      48
#define SYS_ioctl       54
#define SYS_mmap        90
#define SYS_munmap      91
#define SYS_sigreturn   119
#define SYS_clone       120

#define WNOHANG         1
#define INT_MIN         -2147483648

extern char fork_child_trampoline;  // in syscall.asm

int syscall_handler(void);
int syscall_dispatch(struct ucontext *ucontext);
void sys_sigreturn(struct ucontext *ucontext) __attribute__((noreturn));

#endif