#ifndef _TMP_SYSCALL_H
#define _TMP_SYSCALL_H

#include "syscall.h"

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
		: "a"(SYS_wait), "b"(status)
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

#endif