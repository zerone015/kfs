#include "handle.h"
#include "tty.h"
#include "pic.h"
#include "io.h"
#include "stdio.h"

static const char g_key_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
    0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_handle(void)
{
	uint8_t status;
	uint8_t scancode;
	char c;

	status = inb(PS2CTRL_STATUS_PORT);
	if (!(status & 0x01))
		return;
	scancode = inb(PS2CTRL_DATA_PORT);
	if ((scancode & BREAKCODE_MASK) == 0) {
		c = g_key_map[scancode];
		if (c == '\b') {
			tty_delete_input_char();	
		}
		else if (c == '\n') {
			tty_flush_input();
			printf(TTY_PROMPT);
		}
		else if (c && c != 27) {
			tty_insert_input_char(c);
		}
	}
	pic_send_eoi(KEYBOARD_IRQ);
}
