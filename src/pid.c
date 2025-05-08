#include "pid.h"
#include <stdint.h>

uint32_t pidmap[PIDMAP_MAX];

void pidmap_init(void)
{
    *pidmap = 0x3;
}