#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tty.h"
#include "vga.h"
#include "string.h"
#include "io.h"
#include "stdio.h"

static struct tty_context	g_tty[TTY_MAX];
static size_t			g_tty_idx;
static uint16_t			*g_vga_buf;

void tty_init(void)
{
	vga_init();
	for (size_t i = 0; i < TTY_MAX; i++) {
		g_tty[i].row = 0;
		g_tty[i].column = 0;
		g_tty[i].input_length = 0;
		g_tty[i].color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
		g_vga_buf = g_tty[i].buf;
		g_tty_idx = i;
		printf(TTY_HELLO);
		g_tty[i].color = vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
		printf("* Connected to tty%c *\n\n", i + '1');
		g_tty[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		printf(TTY_PROMPT);
		for (size_t j = VGA_WIDTH * g_tty[i].row + g_tty[i].column; j < VGA_SIZE; j++)
			g_vga_buf[j] = vga_entry(' ', g_tty[i].color);
	}
	g_tty_idx = 0;
	g_vga_buf = (uint16_t *) VGA_MEMORY;
	for (size_t i = 0; i < VGA_SIZE; i++)
		g_vga_buf[i] = g_tty[g_tty_idx].buf[i];
}

void tty_set_color(uint8_t color)
{
	struct tty_context *cur_tty;
	
	cur_tty = &g_tty[g_tty_idx];
	cur_tty->color = color;
}

void tty_change(size_t next_tty_idx)
{
	struct tty_context *cur_tty;
	struct tty_context *next_tty;
	size_t cur_tty_size;
	size_t next_tty_size;

	cur_tty = &g_tty[g_tty_idx];
	cur_tty_size = VGA_WIDTH * cur_tty->row + cur_tty->column;
	for (size_t i = 0; i < cur_tty_size; i++)
		cur_tty->buf[i] = g_vga_buf[i];
	next_tty = &g_tty[next_tty_idx];
	next_tty_size = VGA_WIDTH * next_tty->row + next_tty->column;
	for (size_t i = 0; i < next_tty_size; i++)
		g_vga_buf[i] = next_tty->buf[i];
	while (next_tty_size < cur_tty_size) {	
		g_vga_buf[next_tty_size] = vga_entry(' ', next_tty->color);
		next_tty_size++;
	}
	g_tty_idx = next_tty_idx;
	vga_update_cursor(next_tty->column, next_tty->row);
}

void tty_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	size_t idx;

	idx = y * VGA_WIDTH + x;
	g_vga_buf[idx] = vga_entry(c, color);
}

void tty_delete_last_line(void)
{
	struct tty_context *cur_tty;
	size_t idx;

	cur_tty = &g_tty[g_tty_idx];
	idx = VGA_WIDTH * (cur_tty->row - 1);
	for (size_t x = 0; x < VGA_WIDTH; x++)
		g_vga_buf[idx + x] = vga_entry(' ', cur_tty->color);
}

void tty_scroll(void)
{
	struct tty_context *cur_tty;
	size_t non_input_length;

	cur_tty = &g_tty[g_tty_idx];
	non_input_length = VGA_SIZE - cur_tty->input_length;
	if (non_input_length < VGA_WIDTH)
		cur_tty->input_length -= VGA_WIDTH - non_input_length;
	for (size_t i = 0; i < VGA_SIZE - VGA_WIDTH; i++)
		g_vga_buf[i] = g_vga_buf[i + VGA_WIDTH];
	tty_delete_last_line();
	cur_tty->row--;
}

void tty_insert_input_char(char c)
{
	struct tty_context *cur_tty;
	
	tty_putchar(c);
	cur_tty = &g_tty[g_tty_idx];
	cur_tty->input_length++;
}

void tty_delete_input_char(void)
{
	struct tty_context *cur_tty;
	
	cur_tty = &g_tty[g_tty_idx];
	if (cur_tty->input_length > 0) {
		cur_tty->input_length--;
		cur_tty->column--;
		tty_putentryat(' ', cur_tty->color, cur_tty->column, cur_tty->row);
		vga_update_cursor(cur_tty->column, cur_tty->row);
	}
}

void tty_flush_input(void)
{
	struct tty_context *cur_tty;
	uint16_t *input_head;
	size_t input_head_idx;
	char buf[VGA_SIZE];
	size_t i;

	cur_tty = &g_tty[g_tty_idx];
	input_head_idx = VGA_WIDTH * cur_tty->row + cur_tty->column - cur_tty->input_length;
	input_head = &g_vga_buf[input_head_idx];
	for (i = 0; i < cur_tty->input_length; i++)
		buf[i] = input_head[i] & 0xFF;
	tty_putchar('\n');
	tty_write(buf, i);
	if (i > 0) {
		tty_putchar('\n');
		cur_tty->input_length = 0;
	}
}

void tty_putchar(char c) 
{
	struct tty_context *cur_tty;
	
	cur_tty = &g_tty[g_tty_idx];
	if (c == '\n' || cur_tty->column == VGA_WIDTH) {
		cur_tty->column = 0;
		if (++cur_tty->row == VGA_HEIGHT)
			tty_scroll();
	}
	if (c != '\n') {
		tty_putentryat(c, cur_tty->color, cur_tty->column, cur_tty->row);
		cur_tty->column++;
	}
	vga_update_cursor(cur_tty->column, cur_tty->row);
}

void tty_write(const char *data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		tty_putchar(data[i]);
}
