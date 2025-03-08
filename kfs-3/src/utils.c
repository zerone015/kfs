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
