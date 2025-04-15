#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <stdint.h>

static inline uint8_t inb(uint16_t port) 
{
    uint8_t value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "d"(port) : "memory");
    return value;
}

static inline void outb(uint16_t port, uint8_t value) 
{
    __asm__ volatile ("outb %1, %0" :: "d"(port), "a"(value) : "memory");
}

#endif
