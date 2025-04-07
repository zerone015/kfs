#include "pic.h"

void pic_init(void)
{
	pic_remap(PIC1_OFFSET, PIC2_OFFSET);
	irq_clear_mask(KEYBOARD_IRQ);
	// irq_clear_mask(PIT_IRQ);
}

void pic_remap(int offset1, int offset2)
{
	outb(PIC1_CMD_PORT, ICW1_INIT | ICW1_ICW4);
	outb(PIC2_CMD_PORT, ICW1_INIT | ICW1_ICW4);
	outb(PIC1_DATA_PORT, offset1);
	outb(PIC2_DATA_PORT, offset2);
	outb(PIC1_DATA_PORT, 4); 
	outb(PIC2_DATA_PORT, 2);             
	
	outb(PIC1_DATA_PORT, ICW4_8086);
	outb(PIC2_DATA_PORT, ICW4_8086);

	outb(PIC1_DATA_PORT, 0xFF);
	outb(PIC2_DATA_PORT, 0xFF);
}

void irq_set_mask(uint8_t irq_line)
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

void irq_clear_mask(uint8_t irq_line)
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
