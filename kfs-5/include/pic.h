#ifndef _PIC_H
#define _PIC_H

#include <stdint.h>
#include "io.h"

#define PIC1_OFFSET	    0x20		/* IO base address for master PIC */
#define PIC2_OFFSET	    0x28		/* IO base address for slave PIC */
#define PIC1_CMD_PORT	0x20
#define PIC1_DATA_PORT	(PIC1_CMD_PORT + 1)
#define PIC2_CMD_PORT	0xA0
#define PIC2_DATA_PORT	(PIC2_CMD_PORT + 1)
#define PIT_IRQ         0x0
#define KEYBOARD_IRQ	0x01

#define ICW1_ICW4	    0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	    0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	    0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	    0x10		/* Initialization - required! */

#define ICW4_8086	    0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	    0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	    0x10		/* Special fully nested (not) */
#define PIC_EOI		    0x20		/* End-of-interrupt command code */

void pic_init(void);

static inline void pic_send_eoi(uint8_t irq)
{
	if (irq >= 8)
		outb(PIC2_CMD_PORT, PIC_EOI);
	outb(PIC1_CMD_PORT, PIC_EOI);
}

static inline void irq_set_mask(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        irq_line -= 8;
    }
    value = inb(port) | (1 << irq_line);
    outb(port, value);        
}

static inline void irq_clear_mask(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);        
}

#endif
