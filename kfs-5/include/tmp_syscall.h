#ifndef _TMP_SYSCALL_H
#define _TMP_SYSCALL_H

#include "syscall.h"

#define WTERMSIG(status)     ((status) & 0x7F)
#define WIFEXITED(status)    (!WTERMSIG(status))
#define WIFSIGNALED(status)  (!WIFEXITED(status) && ((status) & 0x7F) != 0x7F)
#define WEXITSTATUS(status)  (((status) >> 8) & 0xFF)
#define WIFSTOPPED(status)   (((status) & 0xFF) == 0x7F)
#define WSTOPSIG(status)     (((status) >> 8) & 0xFF)

static inline int __attribute__((always_inline)) fork(void)
{
    int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_fork)
		: "memory"
	);
	return ret;
}

static inline void __attribute__((always_inline)) exit(int status)
{
	__asm__ (
		"int $0x80"
		:
		: "a"(SYS_exit), "b"(status)
		: "memory"
	);
}

static inline int __attribute__((always_inline)) wait(int *status)
{
	int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_waitpid), "b"(-1), "c"(status), "d"(0)
		: "memory"
	);
	return ret;
}

static inline int __attribute__((always_inline)) write(const char *msg)
{
	int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_write), "b"(msg)
		: "memory"
	);
	return ret;
}

static inline int __attribute__((always_inline)) getpid(void)
{
	int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_getpid)
		: "memory"
	);
	return ret;
}

static inline int __attribute__((always_inline)) getuid(void)
{
	int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_getuid)
		: "memory"
	);
	return ret;
}

static inline int __attribute__((always_inline)) kill(int pid, int sig)
{
	int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_kill), "b"(pid), "c"(sig)
		: "memory"
	);
	return ret;
}

static inline int __attribute__((always_inline)) signal(int sig, uintptr_t handler)
{
	int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_signal), "b"(sig), "c"(handler)
		: "memory"
	);
	return ret;
}

#endif