#include "printk.h"
#include "vga.h"
#include "tty.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

static inline __attribute__((always_inline)) void print_hex(va_list *ap, const char *base)
{
	char buf[8];
	unsigned int n;
	int len;

	n = va_arg(*ap, unsigned int);
	len = number_to_string(buf, n, 16, base);
	tty_write(buf, len);
}

static inline __attribute__((always_inline)) void print_u_decimal(va_list *ap)
{
	char buf[10];
	unsigned int n;
	int len;

	n = va_arg(*ap, unsigned int);
	len = nbrlen(n, 10);
	number_to_string(buf, n, 10, "0123456789");
	tty_write(buf, len);
}

static inline __attribute__((always_inline)) void print_decimal(va_list *ap)
{
	char buf[11];
	int n;
	int len;

	n = va_arg(*ap, int);
	len = 0;
	if (n < 0) {
		buf[0] = '-';
		len++;
	}
	len += number_to_string(buf + len, abs(n), 10, "0123456789");
	tty_write(buf, len);
}

static inline __attribute__((always_inline)) void print_address(va_list *ap)
{
	char buf[18];
	size_t n;
	int len;

	n = va_arg(*ap, uintptr_t);
	buf[0] = '0';
	buf[1] = 'x';
	len = number_to_string(buf + 2, n, 16, "0123456789abcdef");
	tty_write(buf, len + 2);
}

static inline __attribute__((always_inline)) void print_str(va_list *ap)
{
	char	*str;
	size_t	len;

	str = va_arg(*ap, char *);
	if (!str)
		str = "(null)";
	len = strlen(str);
	tty_write(str, len);
}

static inline __attribute__((always_inline)) void print_char(va_list *ap)
{
	char	c;

	c = va_arg(*ap, int);
	tty_putchar(c);
}

void printk(const char *__restrict format, ...)
{
	va_list	ap;

	va_start(ap, format);
	if (HAS_LOG_LEVEL(format)) {
		switch (format[1] - '0') {
		case 0:
			tty_set_color(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
			break;
		case 1:
			tty_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
			break;
		case 2:
			tty_set_color(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
			break;
		}
		format += 3;
	}
	while (*format) {
		if (*format == '%') {
			format++;
			switch (*format) {
			case '%':
				tty_putchar('%');
				break;
			case 'X':
				print_hex(&ap, "0123456789ABCDEF");
				break;
			case 'c':
				print_char(&ap);
				break;
			case 'd':
				print_decimal(&ap);
				break;
			case 'i':
				print_decimal(&ap);
				break;
			case 'p':
				print_address(&ap);
				break;
			case 's':
				print_str(&ap);
				break;
			case 'u':
				print_u_decimal(&ap);
				break;
			case 'x':
				print_hex(&ap, "0123456789abcdef");
			}
		}
		else {
			tty_putchar(*format);
		}
		format++;
	}
	tty_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	va_end(ap);
}
