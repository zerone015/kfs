#include "sched.h"
#include "hmm.h"
#include "panic.h"

struct list_head ready_queue;
struct task_struct *current;

static inline void ready_queue_init(void)
{
    init_list_head(&ready_queue);
}

void scheduler_init(void)
{
    ready_queue_init();
}