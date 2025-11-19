#include "printk.h"
#include "errno.h"
#include <stdbool.h>
#include "init.h"
#include "tmp_syscall.h"
#include "exec.h"

static inline void __attribute__((always_inline)) fail(void)
{
	write(KERN_ERR "Failed\n");
	while (true);
}

void init_process_code(void)
{
    int pid, wpid, ret, status;
    
    write("Test: SIGKILL");
    {
        volatile size_t sink = 0;

        pid = fork();
        if (pid == 0) {
			while (true)
            	write("..");
        }
        else {
            while (sink < 10000000)
                sink++;
            ret = kill(pid, SIGKILL);
            wpid = wait(&status);
            if (ret != 0 || wpid != pid || !WIFSIGNALED(status) || WTERMSIG(status) != SIGKILL)
                fail();
        }
    }
	write(KERN_DEBUG "OK (no further output should follow)\n");

    write("Test: signal handler\n");
	{
		ret = signal(SIGINT, TEST_SIGNAL_BASE);
		if (ret < 0)
			write("test failed!\n");
		pid = fork();
        if (pid == 0) {
			while (true);
        }
        else {
            ret = kill(pid, SIGINT);
            if (ret != 0)
                fail();
			while (true);
        }
	}
}

void test_signal_handler(int sig)
{
    write("OK (Signal handler executed successfully. Test passed!)\n");
}

static void test_signal_init(void)
{
    uint32_t *pte;

    pte = pte_from_va(TEST_SIGNAL_BASE);
    for (size_t i = 0; i < (TEST_SIGNAL_SIZE / PAGE_SIZE); i++)
        pte[i] = alloc_pages(PAGE_SIZE) | PG_US | PG_RDWR | PG_PRESENT;

    memcpy((void *)TEST_SIGNAL_BASE, test_signal_handler, TEST_SIGNAL_SIZE);
}