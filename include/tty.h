#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>
#include "vga.h"

#define TTY_HELLO  "   ___  _____                           _  \n" \
              "  /   |/ __  \\                         | | \n" \
              " / /| |`' / /' ___   ___   ___   _   _ | | \n" \
              "/ /_| |  / /  / __| / _ \\ / _ \\ | | | || | \n" \
              "\\___  |./ /___\\__ \\|  __/| (_) || |_| || | \n" \
              "    |_/\\_____/|___/ \\___| \\___/  \\__,_||_| \n" \
              "                                           \n" \
              "                                                   by yoson\n"
#define TTY_MAX			6
#define TTY_PROMPT		"$> "
#define TAB_SIZE		4

struct tty_context {
	uint16_t buf[VGA_SIZE];
	size_t row;
	size_t column;
	uint8_t color;
	size_t input_length;
};

void		tty_init(void);
void		tty_putentryat(char c, uint8_t color, size_t x, size_t y);
void		tty_set_color(uint8_t color);
void		tty_change(size_t target_tty_idx);
void		tty_add_input(char c);
void		tty_delete_input(void);
void		tty_enter_input(void);
void		tty_putchar(char c);
void		tty_write(const char *data, size_t size);
void        tty_clear(void);

#endif
