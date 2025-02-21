#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stdio.h"
#include "tty.h"
#include "vga.h"
#include "pic.h"
#include "idt.h"

static void hello(void)
{
	const char* ascii_art =
    "   ___  _____                           _  \n"
    "  /   |/ __  \\                         | | \n"
    " / /| |`' / /' ___   ___   ___   _   _ | | \n"
    "/ /_| |  / /  / __| / _ \\ / _ \\ | | | || | \n"
    "\\___  |./ /___\\__ \\|  __/| (_) || |_| || | \n"
    "    |_/\\_____/|___/ \\___| \\___/  \\__,_||_| \n"
    "                                           \n\n";
	uint8_t entry_color;

	entry_color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
	tty_set_color(entry_color);
	printf(ascii_art);
	entry_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	tty_set_color(entry_color);
	printf(TTY_PROMPT);
}

static void kernel_init(void)
{
	tty_init();
	idt_init();
	pic_init();
}

void kernel_main(void) 
{
	kernel_init();
	hello();
}
