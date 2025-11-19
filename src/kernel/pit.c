#include "pit.h"
#include "io.h"
#include <stdint.h>

void pit_init(void) 
{
    uint16_t reload;
    
    reload = PIT_FREQUENCY / 1000; 
    outb(PIT_COMMAND, PIT_MODE);
    outb(PIT_CHANNEL0_DATA, reload & 0xFF);
    outb(PIT_CHANNEL0_DATA, reload >> 8);
}