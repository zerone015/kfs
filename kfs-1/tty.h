#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#define TTY_PROMPT "yoson$> "

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
