#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"

extern unsigned int abs(int n);
extern int number_to_string(char *buf, size_t n, size_t radix, const char *base);
extern int nbrlen(size_t n, int radix);
extern size_t strlen(const char *str);
extern void *memset(void *p, int value, size_t size);
extern void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);

#define check_flag(flags,bit)           ((flags) & (1 << (bit)))
#define align_4byte(val)                (((val) + 0x3) & ~0x3)
#define align_page(val)                 (((val) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))
#define align_kpage(val)                (((val) + (K_PAGE_SIZE - 1)) & ~(K_PAGE_SIZE - 1))
#define is_align_kpage(val)				(!((val) & (K_PAGE_SIZE - 1)))
#define container_of(ptr, type, member) ({				\
	    uint8_t *__mptr = (uint8_t *)(ptr);				\
	    ((type *)(__mptr - offsetof(type, member))); })
#define BIT_SET(bitmap, bit)     ((bitmap) |=  (1U << (bit)))
#define BIT_CLEAR(bitmap, bit)   ((bitmap) &= ~(1U << (bit)))

#endif
