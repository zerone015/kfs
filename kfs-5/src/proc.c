#include "proc.h"
#include "sched.h"
#include "hmm.h"
#include "panic.h"
#include "exec.h"
#include "utils.h"
#include "syscall.h"
#include "errno.h"
#include "tmp_syscall.h"
#include "signal.h"

void init_process_code(void)
{
	write("hi~");
	while (true);
}

void init_process(void)
{
	struct task_struct *task;

	task = kmalloc(sizeof(struct task_struct));
	if (!task)
		do_panic("init process create failed");
	
	task->pid = INIT_PROCESS_PID;
	task->uid = 0;
	task->euid = 0;
	task->suid = 0;
	task->pgid = task->pid;
	task->sid = task->pid;
	task->cr3 = current_cr3();
	task->esp0 = (uint32_t)&stack_top;
    task->time_slice_remaining = DEFAULT_TIMESLICE;
    task->parent = NULL;
	task->vblocks.by_base = RB_ROOT;
	task->vblocks.by_size = RB_ROOT;
	task->mapping_files.by_base = RB_ROOT;
	task->state = PROCESS_READY;
	
	task->sig_pending = 0;
	sig_handlers_init(task->sig_handlers);

	init_list_head(&task->children);
	init_list_head(&task->ready);
    
	pid_table_register(task);
    
	current = task;

	exec_fn(init_process_code);
}