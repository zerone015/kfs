#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#define TTY_PROMPT		"yoson$> "
#define TAB_SIZE		4
#define LEFT_SHIFT_PRESS	0x2A
#define RIGHT_SHIFT_PRESS	0x36
#define LEFT_SHIFT_RELEASE	0xAA
#define RIGHT_SHIFT_RELEASE	0xB6

#include <stddef.h>
#include <stdint.h>

void		tty_init(void);
void		tty_set_color(uint8_t color);
void		tty_update_cursor(void);
void		tty_insert_input_char(char c);
void		tty_delete_input_char(void);
void		tty_flush_input(void);
void		tty_putchar(char c);
void		tty_write(const char *data, size_t size);

#endif
