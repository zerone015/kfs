#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tty.h"
#include "vga.h"
#include "utils.h"
#include "io.h"
#include "printk.h"

static struct tty_context	tty[TTY_MAX];
static size_t			cur_tty;
static uint16_t			*vga_buf;

void tty_init(void)
{
	for (size_t i = 0; i < TTY_MAX; i++) {
		tty[i].row = 0;
		tty[i].column = 0;
		tty[i].input_length = 0;
		tty[i].color = VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
		vga_buf = tty[i].buf;
		cur_tty = i;
		printk(TTY_HELLO);
		tty[i].color = VGA_ENTRY_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK);
		printk("* Connected to tty%c *\n\n", i + '1');
		tty[i].color = VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		printk(TTY_PROMPT);
		for (size_t j = VGA_WIDTH * tty[i].row + tty[i].column; j < VGA_SIZE; j++)
			vga_buf[j] = VGA_ENTRY(' ', tty[i].color);
	}
	cur_tty = 0;
	vga_buf = (uint16_t *) VGA_MEMORY;
	for (size_t i = 0; i < VGA_SIZE; i++)
		vga_buf[i] = tty[cur_tty].buf[i];
}

void tty_set_color(uint8_t color)
{
	tty[cur_tty].color = color;
}

void tty_change(size_t next_tty)
{
	size_t cur_tty_size;
	size_t next_tty_size;

	cur_tty_size = VGA_WIDTH * tty[cur_tty].row + tty[cur_tty].column;
	for (size_t i = 0; i < cur_tty_size; i++)
		tty[cur_tty].buf[i] = vga_buf[i];
	next_tty_size = VGA_WIDTH * tty[next_tty].row + tty[next_tty].column;
	for (size_t i = 0; i < next_tty_size; i++)
		vga_buf[i] = tty[next_tty].buf[i];
	while (next_tty_size < cur_tty_size) {	
		vga_buf[next_tty_size] = VGA_ENTRY(' ', tty[next_tty].color);
		next_tty_size++;
	}
	cur_tty = next_tty;
	vga_update_cursor(tty[next_tty].column, tty[next_tty].row);
}

void tty_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	size_t idx;

	idx = y * VGA_WIDTH + x;
	vga_buf[idx] = VGA_ENTRY(c, color);
}

void tty_delete_last_line(void)
{
	size_t idx;

	idx = VGA_WIDTH * (tty[cur_tty].row - 1);
	for (size_t x = 0; x < VGA_WIDTH; x++)
		vga_buf[idx + x] = VGA_ENTRY(' ', tty[cur_tty].color);
}

void tty_scroll(void)
{
	size_t non_input_length;

	non_input_length = VGA_SIZE - tty[cur_tty].input_length;
	if (non_input_length < VGA_WIDTH)
		tty[cur_tty].input_length -= VGA_WIDTH - non_input_length;
	for (size_t i = 0; i < VGA_SIZE - VGA_WIDTH; i++)
		vga_buf[i] = vga_buf[i + VGA_WIDTH];
	tty_delete_last_line();
	tty[cur_tty].row--;
}

void tty_insert_input_char(char c)
{
	tty_putchar(c);
	tty[cur_tty].input_length++;
}

void tty_delete_input_char(void)
{
	if (tty[cur_tty].input_length > 0) {
		tty[cur_tty].input_length--;
		tty[cur_tty].column--;
		tty_putentryat(' ', tty[cur_tty].color, tty[cur_tty].column, tty[cur_tty].row);
		vga_update_cursor(tty[cur_tty].column, tty[cur_tty].row);
	}
}

void tty_flush_input(void)
{
	uint16_t *input_head;
	size_t input_head_idx;
	char buf[VGA_SIZE];
	size_t i;

	input_head_idx = VGA_WIDTH * tty[cur_tty].row + tty[cur_tty].column - tty[cur_tty].input_length;
	input_head = &vga_buf[input_head_idx];
	for (i = 0; i < tty[cur_tty].input_length; i++)
		buf[i] = input_head[i] & 0xFF;
	tty_putchar('\n');
	tty_write(buf, i);
	if (i > 0) {
		tty_putchar('\n');
		tty[cur_tty].input_length = 0;
	}
}

void tty_putchar(char c) 
{
	if (c == '\n' || tty[cur_tty].column == VGA_WIDTH) {
		tty[cur_tty].column = 0;
		if (++(tty[cur_tty].row) == VGA_HEIGHT)
			tty_scroll();
	}
	if (c != '\n') {
		tty_putentryat(c, tty[cur_tty].color, tty[cur_tty].column, tty[cur_tty].row);
		tty[cur_tty].column++;
	}
	vga_update_cursor(tty[cur_tty].column, tty[cur_tty].row);
}

void tty_write(const char *data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		tty_putchar(data[i]);
}
