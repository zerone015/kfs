#include "proc.h"
#include "sched.h"
#include "hmm.h"
#include "panic.h"
#include "exec.h"
#include "unistd.h"
#include "utils.h"

static inline uint32_t current_cr3(void)
{
	uint32_t cr3;

	__asm__ volatile (
		"movl %%cr3, %0"
		: "=r"(cr3)
	);
	return cr3;
}

static inline __attribute__((always_inline)) int t_nbrlen(size_t n, int radix)
{
	int len;

	len = 0;
	if (n == 0)
		len++;
	while (n) {
		len++;
		n /= radix;
	}
	return len;
}

static inline void __attribute__((always_inline)) t_number_to_string(char *buf, size_t n, size_t radix, const char *base)
{
	int i;
	int len;

	len = t_nbrlen(n, radix);
	for (i = 0; i < len; i++) {
		buf[len - 1 - i] = base[n % radix];
		n /= radix;
	}
	buf[len] = '\0';
}

void init_process_code(void)
{
	// char buf[10];
	// int pid;
	// int magic_number = 960705;

	// for (int i = 0; i < 10; i++) {
	// 	pid = fork();
	// 	if (pid < 0) {
	// 		write("fork failed\n");
	// 		while (true);
	// 	}
	// 	if (pid == 0) {
	// 		write("hi i'm child. my pid is");
	// 		t_number_to_string(buf, getuid(), 10, "0123456789");
	// 		write(buf);
	// 		magic_number++;
	// 		write("magic number is ");
	// 		t_number_to_string(buf, magic_number, 10, "0123456789");
	// 		write(buf);
	// 		write("\n");
	// 		while (true);
	// 	}
	// 	else {
	// 		write("fork successed. child pid is");
	// 		t_number_to_string(buf, pid, 10, "0123456789");
	// 		write(buf);
	// 		write("\n");
	// 	}
	// }
	// if (magic_number != 960705)
	// 	write("cow failed.. OTL\n");
	// else
	// 	write("um... for the time being multi tasking ok..");
	// while(true);
	
	int ret;

    while (42) {
        __asm__ volatile (
            "int $0x80"
            : "=a"(ret)
            : "a"(4), "b"("syscall test!!\n")
        );
		while (true)
		;
    }
}

void init_process(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(struct task_struct));
	if (!task)
		do_panic("init process create failed");
	
	task->pid = INIT_PROCESS_PID;
	task->uid = task->pid;				//stub
	task->cr3 = current_cr3();
	task->esp0 = (uint32_t)&stack_top;
    task->time_slice_remaining = DEFAULT_TIMESLICE;
    task->parent = NULL;
	task->vblocks.by_base = RB_ROOT;
	task->vblocks.by_size = RB_ROOT;
	task->mapping_files.by_base = RB_ROOT;
	task->state = PROCESS_RUNNING;
	
	init_list_head(&task->children);
	init_list_head(&task->ready);
    
	pid_table_register(task);
    
	current = task;
	
	exec_fn(init_process_code);
}