#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <stdint.h>

static inline uint8_t inb(uint16_t port) 
{
    uint8_t value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "d"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) 
{
    __asm__ ("outb %1, %0" :: "d"(port), "a"(value));
}

static inline uint16_t inw(uint16_t port) 
{
    uint16_t value;

    __asm__ volatile ("inw %1, %0" : "=a"(value) : "d"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) 
{
    __asm__ ("outw %1, %0" :: "d"(port), "a"(value));
}

static inline uint32_t inl(uint16_t port) 
{
    uint32_t value;

    __asm__ volatile ("inl %1, %0" : "=a"(value) : "d"(port));
    return value;
}

static inline void outl(uint16_t port, uint32_t value) 
{
    __asm__ ("outl %1, %0" :: "d"(port), "a"(value));
}

#endif
