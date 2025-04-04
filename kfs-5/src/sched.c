#include "sched.h"

static struct pid_allocator pid_alloc;

static inline void __set_pid(int pid)
{
    BIT_SET(pid_alloc.pidmap[pid / 32], pid % 32);
}

static inline void __clear_pid(int pid)
{
    BIT_CLEAR(pid_alloc.pidmap[pid / 32], pid % 32);
}

static inline int __find_free_pid(void)
{
    int bit_offset;

    for (int i = 0; i < PIDMAP_MAX; i++) {
        if (~pid_alloc.pidmap[i]) {
            bit_offset = __builtin_ctz(~pid_alloc.pidmap[i]);
            BIT_SET(pid_alloc.pidmap[i], bit_offset);
            return bit_offset + 32*i;
        }
    }
    return -1;
}

int alloc_pid(void)
{
    int pid;

    if (pid_alloc.useful_pid != -1) {
        pid = pid_alloc.useful_pid;
        pid_alloc.useful_pid = -1;
        __set_pid(pid);
        return pid;
    }
    return __find_free_pid();
}

void free_pid(int pid)
{
    if (pid_alloc.useful_pid == -1)
        pid_alloc.useful_pid = pid;
    else
        __clear_pid(pid);
}