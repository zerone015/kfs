#include "sched.h"
#include "hmm.h"
#include "panic.h"
#include "exec.h"

struct task_struct *current;
struct task_struct *pid_table[PID_TABLE_MAX];
