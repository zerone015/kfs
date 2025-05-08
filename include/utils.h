#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "paging.h"

#define check_flag(flags, bit)          ((flags) & (1 << (bit)))
#define align_up(val, size)       		(((val) + ((size) - 1)) & ~((size) - 1))
#define align_down(val, size)       	((val) & ~((size) - 1))
#define is_aligned(val, size)			(!((val) & ((size) - 1)))
#define container_of(ptr, type, member) ({				\
	    uint8_t *__mptr = (uint8_t *)(ptr);				\
	    ((type *)(__mptr - offsetof(type, member))); })
#define BIT_SET(bitmap, offset)     	((bitmap) |=  (1U << (offset)))
#define BIT_CLEAR(bitmap, offset)   	((bitmap) &= ~(1U << (offset)))
#define BIT_CHECK(bits, offset)			((bits) & (1U << (offset)))

#define __STR(x) 						#x
#define STR(x) 							__STR(x)

unsigned int abs(int n);
int number_to_string(char *buf, size_t n, size_t radix, const char *base);
int nbrlen(size_t n, int radix);
size_t strlen(const char *str);
void *memset(void *p, int value, size_t size);
void *memset32(void *p, size_t value, size_t count) ;
void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
void *memcpy32(void *restrict dstptr, const void *restrict srcptr, size_t count) ;

#endif
