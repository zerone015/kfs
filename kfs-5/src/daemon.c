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
	struct task_struct *proc;

	if (!(proc = kmalloc(sizeof(struct task_struct))))
		do_panic("init process create failed");
	
	proc->pid = INIT_PROCESS_PID;
	if (!pgroup_create(proc))
		do_panic("init process create failed");

	proc->cr3 = current_cr3();
	proc->esp0 = (uint32_t)&stack_top;
	proc->state = PROCESS_READY;
    proc->time_slice_remaining = DEFAULT_TIMESLICE;
    proc->parent = NULL;
	proc->uid = 0;
	proc->euid = 0;
	proc->suid = 0;
	proc->sid = proc->pid;
	proc->vblocks.by_base = RB_ROOT;
	proc->vblocks.by_size = RB_ROOT;
	proc->mapping_files.by_base = RB_ROOT;
	
	proc->sig_pending = 0;
	signal_init(proc->sig_handlers);

	init_list_head(&proc->children);
	init_list_head(&proc->ready);
    
	process_register(proc);
    
	current = proc;

	exec_fn(init_process_code);
}