#include "utils.h"

unsigned int abs(int n)
{
	if (n < 0)
		n *= -1;
	return n;
}

int number_to_string(char *buf, size_t n, size_t radix, const char *base)
{
	int i;
	int len;

	len = nbrlen(n, radix);
	for (i = 0; i < len; i++) {
		buf[len - 1 - i] = base[n % radix];
		n /= radix;
	}
	return i;
}

int nbrlen(size_t n, int radix)
{
	int len;

	len = 0;
	if (n == 0)
		len++;
	while (n) {
		len++;
		n /= radix;
	}
	return len;
}

size_t strlen(const char *str)
{
	size_t len;

	len = 0;
	while (str[len])
		len++;
	return len;
}

void *memset(void *p, int value, size_t size) 
{
	unsigned char *buf;
	
	buf = (unsigned char *) p;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char)value;
	return p;
}

void *memset32(void *p, size_t value, size_t count) 
{
	size_t *buf;
	
	buf = (size_t *)p;
	for (size_t i = 0; i < count; i++)
		buf[i] = (size_t)value;
	return p;
}

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size) 
{
	unsigned char *dst = (unsigned char *)dstptr;
	const unsigned char *src = (const unsigned char *)srcptr;
	
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}
