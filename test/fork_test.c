#include "printk.h"
#include "errno.h"
#include <stdbool.h>
#include "init.h"
#include "tmp_syscall.h"

#define FORK_MAGIC_NUMBER	1234567

static inline void __attribute__((always_inline)) fork_return_error(int pid)
{
	write(KERN_ERR "Failed\n");
	if (pid == -ENOMEM)
		write("memory not enough\n");
	else if (pid == -EAGAIN)
		write("pid allocate failed\n");
	while (true);
}

static inline void __attribute__((always_inline)) fail(void)
{
	write(KERN_ERR "fork test failed!\n");
	while (true);
}

static inline void __attribute__((always_inline)) endless_check(volatile int *result, const int expect)
{
	while (true) {
		if (*result != expect)
			fail();
	}
}

void init_process_code(void)
{
	{
		write("parent creates many children in a loop (if successful, nothing is printed)\n");

		int pid;
		int child_pid = INIT_PROCESS_PID + 1;
		volatile int magic_number = FORK_MAGIC_NUMBER;

		for (int i = 0; i < 1000; i++) {
			pid = fork();
			if (pid < 0) {
				fork_return_error(pid);
			} if (pid == 0) {
				magic_number++;
				if (getpid() != child_pid || magic_number != FORK_MAGIC_NUMBER + 1)
					fail();
				while (true);
			} else {
				if (pid != child_pid)
					fail();
			}
			child_pid++;
		}
		endless_check(&magic_number, FORK_MAGIC_NUMBER);
	}

	// {
	// 	write("child process performs another fork (If successful, nothing is printed)\n");

	// 	int pid;
	// 	int child_pid = INIT_PROCESS_PID + 1;
	// 	volatile int magic_number = FORK_MAGIC_NUMBER;
	
	// 	pid = fork();
	// 	if (pid < 0)
	// 		fork_return_error(pid);
	// 	if (pid == 0) {
	// 		magic_number++;
	// 		if (getpid() != child_pid || magic_number != FORK_MAGIC_NUMBER + 1)
	// 			fail();
	// 		child_pid++;
	// 		pid = fork();
	// 		if (pid < 0)
	// 			fork_return_error(pid);
	// 		if (pid == 0) {
	// 			magic_number++;
	// 			if (getpid() != child_pid || magic_number != FORK_MAGIC_NUMBER + 2)
	// 				fail();
	// 			while (true);
	// 		}
	// 		else {
	// 			endless_check(&magic_number, FORK_MAGIC_NUMBER + 1);
	// 		}
	// 	}
	// 	endless_check(&magic_number, FORK_MAGIC_NUMBER);
	// }
}