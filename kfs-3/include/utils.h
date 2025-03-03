#ifndef _UTILS_H
#define _UTILS_H

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))
#define CHAR_TO_DIGIT(c)	((c) - '0')

#include <stdint.h>
#include <stddef.h>

unsigned int abs(int n);
int number_to_string(char *buf, size_t n, size_t radix, const char *base);
int nbrlen(size_t n, int radix);
size_t strlen(const char *str);

#endif
