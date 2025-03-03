#ifndef _VGA_H
#define _VGA_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH		80
#define VGA_HEIGHT		25
#define VGA_SIZE		(VGA_WIDTH * VGA_HEIGHT)
#define VGA_MEMORY		0xC03FF000

#define CRTC_ADDRESS_PORT	0x3D4
#define CRTC_DATA_PORT		0x3D5
#define CURSOR_HIGH_LOCATION	0x0E
#define CURSOR_LOW_LOCATION	0x0F
#define CURSOR_START_IDX	0x0A
#define CURSOR_END_IDX		0x0B

#define VGA_INPUT_STATUS_PORT	0x3DA
#define VGA_ADDRESS_DATA_PORT	0x3C0
#define VGA_DATA_READ_PORT	0x3C1
#define VGA_ATTR_IDX		0x10
#define VGA_ATTR_BLINK_BIT	3

#define VGA_ENABLE_BLINK	1
#define VGA_DISABLE_BLINK	0

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

void vga_init(void);
void vga_set_blink(int flag);
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void vga_update_cursor(size_t x, size_t y);

#endif
