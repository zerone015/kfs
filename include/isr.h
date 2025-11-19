#ifndef _ISR_H
#define _ISR_H

#include <stdint.h>
#include "paging.h"
#include "panic.h"

struct interrupt_frame {
    uint32_t edi, esi, ebp, _esp_dummy, ebx, edx, ecx, eax;
    uint32_t error_code, eip, cs, eflags;
    uint32_t esp, ss;
};

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

void division_error_handler(void);
void division_error_handle(void);
void debug_handler(void);
void debug_handle(void);
void nmi_handler(void);
void breakpoint_handler(void);
void overflow_handler(void);
void bound_range_handler(void);
void invalid_opcode_handler(void);
void invalid_opcode_handle(void);
void device_not_avail_handler(void);
void device_not_avail_handle(void);
void double_fault_handler(void);
void invalid_tss_handler(void);
void segment_not_present_handler(void);
void stack_fault_handler(void);
void gpf_handler(void);
void gpf_handle(void);
void page_fault_handler(void);
void page_fault_handle(uintptr_t fault_addr, int error_code);
void floating_point_handler(void);
void alignment_check_handler(void);
void machine_check_handler(void);
void simd_floating_point_handler(void);
void pit_handler(void);
void pit_handle(void);
void keyboard_handler(void);
void keyboard_handle(void);

#endif