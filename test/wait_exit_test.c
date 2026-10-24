#include "printk.h"
#include "tmp_syscall.h"
#include "errno.h"
#include "test.h"

#ifndef WAIT_EXIT_LOOPS
#define WAIT_EXIT_LOOPS		10000000
#endif

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
	write(KERN_ERR "Failed\n");
	while (true);
}

void init_process_code(void)
{
	int pid, wait_pid, status;

	write("Test: wait and _exit stress: ");
	for (int i = 0; i < WAIT_EXIT_LOOPS; i++) {
		pid = fork();
		if (pid < 0)
			fork_return_error(pid);
		if (pid == 0) {
			exit(42);
			fail();
		}
		else {
			wait_pid = wait(&status);
			if (wait_pid != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 42)
				fail();
		}
	}
	write(KERN_DEBUG "OK\n");
	write(TEST_DONE_MARKER);
	while (true);
}