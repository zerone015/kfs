#ifndef _HANDLE_H
#define _HANDLE_H

#include <stdint.h>

#define PS2CTRL_DATA_PORT	0x60
#define PS2CTRL_STATUS_PORT	0x64
#define Ps2CTRL_CMD_PORT	0x64
#define BREAKCODE_MASK		0x80

void keyboard_handler(void);
void keyboard_handle(void);

#endif
