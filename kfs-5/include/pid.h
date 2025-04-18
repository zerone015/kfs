#ifndef _PID_H
#define _PID_H

#include <stdint.h>
#include "utils.h"

#define PID_MAX     32768
#define PIDMAP_MAX  (PID_MAX / 8 / 4)
#define PID_NONE    -1

extern uint32_t pidmap[PIDMAP_MAX];

void pidmap_init(void);

static inline int alloc_pid(void)
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

static inline void free_pid(int pid)
{
    BIT_CLEAR(pidmap[pid / 32], pid % 32);
}

#endif