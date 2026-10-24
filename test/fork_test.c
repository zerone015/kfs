#include "printk.h"
#include "errno.h"
#include <stdbool.h>
#include "init.h"
#include "tmp_syscall.h"
#include "test.h"

#define FORK_MAGIC_NUMBER	1234567
#define FORK_CHILDREN		1000

static inline void __attribute__((always_inline)) fork_return_error(int pid)
{
	write(KERN_ERR "Failed\n");
	if (pid == -ENOMEM)
		write("memory not enough\n");
	else if (pid == -EAGAIN)
		write("pid allocate failed\n");
	while (true);
}

static inline void __attribute__((always_inline)) fail(const char *msg)
{
	write(KERN_ERR "Failed: ");
	write(msg);
	while (true);
}

static inline void __attribute__((always_inline)) reap_children(int count, int expect_status)
{
	int status;

	for (int i = 0; i < count; i++) {
		if (wait(&status) < 0)
			fail("wait\n");
		if (!WIFEXITED(status) || WEXITSTATUS(status) != expect_status)
			fail("child reported an error\n");
	}
	if (wait(&status) != -ECHILD)
		fail("unexpected child left\n");
}

void init_process_code(void)
{
	write("Test: parent creates many children: ");
	{
		int pid;
		int child_pid = INIT_PROCESS_PID + 1;
		volatile int magic_number = FORK_MAGIC_NUMBER;

		for (int i = 0; i < FORK_CHILDREN; i++) {
			pid = fork();
			if (pid < 0)
				fork_return_error(pid);
			if (pid == 0) {
				magic_number++;
				if (getpid() != child_pid || magic_number != FORK_MAGIC_NUMBER + 1)
					exit(1);
				exit(0);
			}
			if (pid != child_pid)
				fail("unexpected child pid\n");
			child_pid++;
		}
		reap_children(FORK_CHILDREN, 0);
		if (magic_number != FORK_MAGIC_NUMBER)
			fail("parent memory changed by child\n");
	}
	write(KERN_DEBUG "OK\n");

	write("Test: child performs another fork: ");
	{
		int pid;
		int child_pid = INIT_PROCESS_PID + 1;
		volatile int magic_number = FORK_MAGIC_NUMBER;

		pid = fork();
		if (pid < 0)
			fork_return_error(pid);
		if (pid == 0) {
			magic_number++;
			if (getpid() != child_pid || magic_number != FORK_MAGIC_NUMBER + 1)
				exit(1);
			child_pid++;
			pid = fork();
			if (pid < 0)
				exit(1);
			if (pid == 0) {
				magic_number++;
				if (getpid() != child_pid || magic_number != FORK_MAGIC_NUMBER + 2)
					exit(1);
				exit(0);
			}
			int status;
			if (wait(&status) != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
				exit(1);
			if (magic_number != FORK_MAGIC_NUMBER + 1)
				exit(1);
			exit(0);
		}
		reap_children(1, 0);
		if (magic_number != FORK_MAGIC_NUMBER)
			fail("parent memory changed by child\n");
	}
	write(KERN_DEBUG "OK\n");

	write(TEST_DONE_MARKER);
	while (true);
}