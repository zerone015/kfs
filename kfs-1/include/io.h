#ifndef _ASM_IO_H
# define _ASM_IO_H

#include <stdint.h>

void	outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#endif
