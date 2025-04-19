#include "unistd.h"
#include "syscall.h"

int fork(void)
{
    int ret;

	__asm__ volatile (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_fork)
	);
	return ret;
}

void exit(int status)
{
	__asm__ (
		"int $0x80"
		:
		: "a"(SYS_exit), "b"(status)
	);
}

int wait(int *status)
{
	int ret;

	__asm__ (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_wait), "b"(status)
	);
	return ret;
}

void write(const char *msg)
{
	__asm__ (
		"int $0x80"
		: 
		: "a"(SYS_write), "b"(msg)
	);
}

int getuid(void)
{
	int ret;

	__asm__ (
		"int $0x80"
		: "=a"(ret)
		: "a"(SYS_getuid)
	);
	return ret;
}