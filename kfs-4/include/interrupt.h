#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>
#include "panic.h"

/* keyboard handler */
#define PS2CTRL_DATA_PORT	0x60
#define PS2CTRL_STATUS_PORT	0x64
#define PS2CTRL_CMD_PORT	0x64
#define BREAKCODE_MASK		0x80
#define LEFT_SHIFT_PRESS	0x2A
#define RIGHT_SHIFT_PRESS	0x36
#define LEFT_SHIFT_RELEASE	0xAA
#define RIGHT_SHIFT_RELEASE	0xB6
#define F1_PRESS            0x3B
#define F6_PRESS            0x40

/* page fault handler */
#define K_NO_PRESENT_MASK 0x5

extern void page_fault_handler(void);
extern void page_fault_handle(uint32_t error_code, struct panic_info info);
extern void keyboard_handler(void);
extern void keyboard_handle(void);
extern void division_error_handler(void);
extern void division_error_handle(struct panic_info info);

#endif