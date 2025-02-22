#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tty.h"
#include "vga.h"
#include "string.h"
#include "io.h"

size_t		g_tty_row;
size_t		g_tty_column;
uint8_t		g_tty_color;
uint16_t	*g_tty_buffer;
size_t		g_tty_input_len;

/*
 * This function disables the blink bit at the start of terminal initialization,
 * allowing the use of 16 background colors. However, some emulators may not 
 * emulate blinking, and in such cases, the terminal supports the default 16 colors.
 */
void tty_init(void)
{
	vga_enable_16color_background();
	g_tty_row = 0;
	g_tty_column = 0;
	g_tty_input_len = 0;
	g_tty_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	g_tty_buffer = (uint16_t *) VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t idx = y * VGA_WIDTH + x;
			g_tty_buffer[idx] = vga_entry(' ', g_tty_color);
		}
	}
}

void tty_update_cursor(void)
{
	uint16_t pos;

	pos = VGA_WIDTH * g_tty_row + g_tty_column;
	outb(CRTC_ADDRESS_PORT, CURSOR_LOW_LOCATION);
	outb(CRTC_DATA_PORT, (uint8_t) (pos & 0xFF));
	outb(CRTC_ADDRESS_PORT, CURSOR_HIGH_LOCATION);
	outb(CRTC_DATA_PORT, (uint8_t) ((pos >> 8) & 0xFF));
}

void tty_set_color(uint8_t color)
{
	g_tty_color = color;
}

static void tty_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t idx = y * VGA_WIDTH + x;
	g_tty_buffer[idx] = vga_entry(c, color);
}

static void tty_delete_last_line(void)
{
	for (size_t x = 0; x < VGA_WIDTH; x++) {
		const size_t idx = VGA_WIDTH * (VGA_HEIGHT - 1) + x;
		g_tty_buffer[idx] = vga_entry(' ', g_tty_color);
	}
}

static void tty_scroll(void)
{
	size_t non_input_len;

	non_input_len = VGA_WIDTH * VGA_HEIGHT - g_tty_input_len;
	if (non_input_len < VGA_WIDTH)
		g_tty_input_len -= VGA_WIDTH - non_input_len;
	for (size_t y = 1; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t idx = y * VGA_WIDTH + x;
			g_tty_buffer[idx - VGA_WIDTH] = g_tty_buffer[idx];
		}
	}
	tty_delete_last_line();
	g_tty_row--;
}

void tty_insert_input_char(char c)
{
	tty_putchar(c);
	g_tty_input_len++;
}

void tty_delete_input_char(void)
{
	if (g_tty_input_len > 0) {
		const size_t idx = g_tty_row * VGA_WIDTH + g_tty_column - 1; 
		g_tty_buffer[idx] = ' ';
		g_tty_input_len--;
		g_tty_column--;
		tty_update_cursor();
	}
}

void tty_flush_input(void)
{
	char buffer[VGA_WIDTH * VGA_HEIGHT];
	uint16_t *input_head;
	size_t input_head_idx;
	size_t i;

	input_head_idx = g_tty_row * VGA_WIDTH + g_tty_column - g_tty_input_len;
	input_head = &g_tty_buffer[input_head_idx];
	for (i = 0; i < g_tty_input_len; i++)
		buffer[i] = input_head[i] & 0xFF;
	tty_putchar('\n');
	for (size_t j = 0; j < i; j++)
		tty_putchar(buffer[j]);
	if (i > 0) {
		tty_putchar('\n');
		g_tty_input_len = 0;
	}
}

void tty_putchar(char c) 
{
	if (c == '\n' || g_tty_column == VGA_WIDTH) {
		g_tty_column = 0;
		if (++g_tty_row == VGA_HEIGHT)
			tty_scroll();
	}
	if (c != '\n') {
		tty_putentryat(c, g_tty_color, g_tty_column, g_tty_row);
		g_tty_column++;
	}
	tty_update_cursor();
}

void tty_write(const char *data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		tty_putchar(data[i]);
}
