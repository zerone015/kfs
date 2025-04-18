#include "proc.h"
#include "sched.h"
#include "hmm.h"
#include "panic.h"
#include "exec.h"

static inline uint32_t current_cr3(void)
{
	uint32_t cr3;

	__asm__ volatile (
		"movl %%cr3, %0"
		: "=r"(cr3)
	);
	return cr3;
}

void init_process_code(void)
{
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