#include "printk.h"
#include "tmp_syscall.h"
#include "errno.h"

static inline void __attribute__((always_inline)) fork_return_error(int pid)
{
	write(KERN_ERR "Failed\n");
	if (pid == -ENOMEM)
		write("memory not enough\n");
	else if (pid == -EAGAIN)
		write("pid allocate failed\n");
	while (true);
}

static inline void __attribute__((always_inline)) success(void)
{
	write(KERN_DEBUG "OK\n");
	while (true);
}

static inline void __attribute__((always_inline)) fail(void)
{
	write(KERN_ERR "Failed\n");
	while (true);
}

static inline void __attribute__((always_inline)) if_reached_fail(void)
{
	fail();
}

void init_process_code(void)
{
	int pid, wait_pid, status;

	write("wait and _exit test: ");
	for (int i = 0; i < 10000000; i++) {
		pid = fork();
		if (pid < 0)
			fork_return_error(pid);
		if (pid == 0) {
			exit(42);
			if_reached_fail();
		}
		else {
			wait_pid = wait(&status);
			if (wait_pid != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 42)
				fail();
		}
	}
	success();
}
