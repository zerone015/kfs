#include "sched.h"
#include "hmm.h"
#include "panic.h"

struct task_struct *current;
struct task_struct *pid_table[PID_TABLE_MAX];
struct list_head ready_queue;

static inline void ready_queue_init(void)
{
    init_list_head(&ready_queue);
}

void scheduler_init(void)
{
    ready_queue_init();
}