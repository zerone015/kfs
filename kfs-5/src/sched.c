#include "sched.h"

static uint32_t pidmap[PIDMAP_MAX];

int alloc_pid(void)
{
    int offset;

    for (int i = 0; i < PIDMAP_MAX; i++) {
        if (~pidmap[i]) {
            offset = __builtin_ctz(~pidmap[i]);
            BIT_SET(pidmap[i], offset);
            return offset + 32*i;
        }
    }
    return PID_NONE;
}

void free_pid(int pid)
{
    BIT_CLEAR(pidmap[pid / 32], pid % 32);
}