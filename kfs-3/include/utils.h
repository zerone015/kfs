#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <stddef.h>

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

unsigned int abs(int n);
int number_to_string(char *buf, size_t n, size_t radix, const char *base);
int nbrlen(size_t n, int radix);
size_t strlen(const char *str);
void *memset(void *p, int value, size_t size);
void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);

static inline void reload_cr3(void) 
{
    uint32_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile ("mov %0, %%cr3" :: "r" (cr3));
}

#endif
