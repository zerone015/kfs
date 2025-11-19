#include "vga.h"
#include "io.h"

void vga_init(void)
{
	vga_set_blink(VGA_DISABLE_BLINK);
	vga_enable_cursor(13, 15);
}

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

void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(CRTC_ADDRESS_PORT, CURSOR_START_IDX);
	outb(CRTC_DATA_PORT, (inb(CRTC_DATA_PORT) & 0xC0) | cursor_start);

	outb(CRTC_ADDRESS_PORT, CURSOR_END_IDX);
	outb(CRTC_DATA_PORT, (inb(CRTC_DATA_PORT) & 0xE0) | cursor_end);
}

void vga_disable_cursor(void)
{
	outb(CRTC_ADDRESS_PORT, CURSOR_START_IDX);
	outb(CRTC_DATA_PORT, CURSOR_DISABLE_BIT);
}

void vga_update_cursor(size_t x, size_t y)
{
	size_t pos;

	pos = VGA_WIDTH * y + x;
	outb(CRTC_ADDRESS_PORT, CURSOR_LOW_LOCATION);
	outb(CRTC_DATA_PORT, (uint8_t) (pos & 0xFF));
	outb(CRTC_ADDRESS_PORT, CURSOR_HIGH_LOCATION);
	outb(CRTC_DATA_PORT, (uint8_t) (pos >> 8));
}
