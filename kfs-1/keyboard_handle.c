#include "handle.h"
#include "tty.h"
#include "pic.h"
#include "io.h"
#include "stdio.h"

/* This is valid only for US QWERTY keyboards. */
static const char g_key_map[128] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

/* This is valid only for US QWERTY keyboards. */
static const char g_shift_key_map[128] =
{
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', /* <-- Tab */
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, /* <-- control key */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',  0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

static uint8_t g_shift_flag = 0;

void keyboard_handle(void)
{
	uint8_t keycode;
	char c;

	keycode = inb(PS2CTRL_DATA_PORT);
	if ((keycode & BREAKCODE_MASK) == 0) {
		c = g_key_map[keycode];
		if (keycode == LEFT_SHIFT_PRESS || keycode == RIGHT_SHIFT_PRESS) {
			g_shift_flag = 1;
		}
		else if (c == '\b') {
			tty_delete_input_char();	
		}
		else if (c == '\n') {
			tty_flush_input();
			printf(TTY_PROMPT);
		}
		else if (c == '\t') {
			for (size_t i = 0; i < TAB_SIZE; i++)
				tty_insert_input_char(' ');
		}
		else if (c) {
			if (g_shift_flag)
				c = g_shift_key_map[keycode];
			tty_insert_input_char(c);
		}
	} else {
		if (keycode == LEFT_SHIFT_RELEASE || keycode == RIGHT_SHIFT_RELEASE)
			g_shift_flag = 0;
	}
	pic_send_eoi(KEYBOARD_IRQ);
}
