#include "sched.h"
#include "hmm.h"
#include "panic.h"

static struct list_head ready_queue;
struct task_struct *current;

static inline void __ready_queue_init(void)
{
    init_list_head(&ready_queue);
}

void scheduler_init(void)
{
    __ready_queue_init();
}

void schedule_process(struct task_struct *ts)
{
    ts->state = PROCESS_READY;
    list_add_tail(&ts->ready, &ready_queue);
}