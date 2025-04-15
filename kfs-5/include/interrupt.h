#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>
#include "paging.h"
#include "panic.h"

struct interrupt_frame {
    uint32_t edi, esi, ebp, _esp_dummy, ebx, edx, ecx, eax;
    uint32_t error_code, eip, cs, eflags;
    uint32_t esp, ss;
};

#define panic_is_user(cs)     (((cs) & 0x3) == 3)

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
#define PF_EC_PRESENT               PG_PRESENT
#define PF_EC_USER                  PG_US
#define PF_PDE_FLAGS_MASK           0x17FF
#define PF_PTE_FLAGS_MASK           0x1FF
#define pf_is_user(ec)              ((ec) & PF_EC_USER)
#define pg_is_reserve(ec, entry)    (!((ec) & PF_EC_PRESENT) && ((entry) & PG_RESERVED))
#define MAKE_PRESENT_PDE(p)         ((p) = (alloc_pages(K_PAGE_SIZE) | (((p) & PF_PDE_FLAGS_MASK) | PG_PRESENT)))
#define MAKE_PRESENT_PTE(p)         ((p) = (alloc_pages(PAGE_SIZE) | (((p) & PF_PTE_FLAGS_MASK) | PG_PRESENT)))

void division_error_handler(void);
void division_error_handle(struct interrupt_frame iframe);
void debug_handler(void);
void debug_handle(struct interrupt_frame iframe);
void nmi_handler(void);
void nmi_handle(struct interrupt_frame iframe);
void breakpoint_handler(void);
void breakpoint_handle(struct interrupt_frame iframe);
void overflow_handler(void);
void overflow_handle(struct interrupt_frame iframe);
void bound_range_handler(void);
void bound_range_handle(struct interrupt_frame iframe);
void invalid_opcode_handler(void);
void invalid_opcode_handle(struct interrupt_frame iframe);
void device_not_avail_handler(void);
void device_not_avail_handle(struct interrupt_frame iframe);
void double_fault_handler(void);
void double_fault_handle(struct interrupt_frame iframe);
void coprocessor_handler(void);
void coprocessor_handle(struct interrupt_frame iframe);
void invalid_tss_handler(void);
void invalid_tss_handle(struct interrupt_frame iframe);
void segment_not_present_handler(void);
void segment_not_present_handle(struct interrupt_frame iframe);
void stack_fault_handler(void);
void stack_fault_handle(struct interrupt_frame iframe);
void gpf_handler(void);
void gpf_handle(struct interrupt_frame iframe);
void page_fault_handler(void);
void page_fault_handle(struct interrupt_frame iframe);
void floating_point_handler(void);
void floating_point_handle(struct interrupt_frame iframe);
void alignment_check_handler(void);
void alignment_check_handle(struct interrupt_frame iframe);
void machine_check_handler(void);
void machine_check_handle(struct interrupt_frame iframe);
void simd_floating_point_handler(void);
void simd_floating_point_handle(struct interrupt_frame iframe);
void virtualization_handler(void);
void virtualization_handle(struct interrupt_frame iframe);
void control_protection_handler(void);
void control_protection_handle(struct interrupt_frame iframe);
void fpu_error_handler(void);
void fpu_error_handle(struct interrupt_frame iframe);
void pit_handler(void);
void pit_handle(struct interrupt_frame iframe);
void keyboard_handler(void);
void keyboard_handle(void);
int syscall_handler(void);
int syscall_handle(struct interrupt_frame iframe);

#endif