#include <stdarg.h>
#include <limits.h>
#include <stdbool.h>
#include "stdio.h"
#include "tty.h"
#include "string.h"

static void print(const char *data, size_t length) 
{
	const unsigned char *bytes = (const unsigned char *) data;
	for (size_t i = 0; i < length; i++)
		tty_putchar(bytes[i]);
}

int printf(const char *restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				return -1;
			}
			print(format, amount);
			format += amount;
			written += amount;
			continue;
		}
		const char* format_begun_at = format++;
		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int);
			if (!maxrem) 
				return -1;
			print(&c, sizeof(c));
			written++;
		} else if (*format == 's') {
			format++;
			const char *str = va_arg(parameters, const char *);
			size_t len = strlen(str);
			if (maxrem < len)
				return -1;
			print(str, len);
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len)
				return -1;
			print(format, len);
			written += len;
			format += len;
		}
	}
	va_end(parameters);
	return written;
}
