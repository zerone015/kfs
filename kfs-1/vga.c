#include "vga.h"
#include "io.h"

void vga_set_blink(int flag)
{
	uint8_t prev_idx;
	uint8_t prev_value;

	inb(VGA_INPUT_STATUS_PORT);
	prev_idx = inb(VGA_ADDRESS_DATA_PORT);
	outb(VGA_ADDRESS_DATA_PORT, VGA_ATTR_IDX);
	prev_value = inb(VGA_DATA_READ_PORT);
	if (flag == VGA_ENABLE_BLINK)
		outb(VGA_ADDRESS_DATA_PORT, prev_value | 1 << VGA_ATTR_BLINK_BIT);
	else
		outb(VGA_ADDRESS_DATA_PORT, prev_value & ~(1 << VGA_ATTR_BLINK_BIT));
	outb(VGA_ADDRESS_DATA_PORT, prev_idx);
}

