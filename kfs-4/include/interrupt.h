#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>

struct general_purpose_registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
};

struct interrupt_frame {
    struct general_purpose_registers gpr;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
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

/* page fault handler */
#define EC_PG_PRESENT               PG_PRESENT
#define __is_reserve(ec, entry)     (!((ec) & EC_PG_PRESENT) && ((entry) & PG_RESERVED))
#define __make_pde(entry)        (alloc_pages(K_PAGE_SIZE) + (((entry) & 0x17FF) | 0x1))

extern void panic(const char *msg, struct interrupt_frame *iframe);

extern void division_error_handler(void);
extern void division_error_handle(struct interrupt_frame iframe);
extern void debug_handler(void);
extern void debug_handle(struct interrupt_frame iframe);
extern void nmi_handler(void);
extern void nmi_handle(struct interrupt_frame iframe);
extern void breakpoint_handler(void);
extern void breakpoint_handle(struct interrupt_frame iframe);
extern void overflow_handler(void);
extern void overflow_handle(struct interrupt_frame iframe);
extern void bound_range_handler(void);
extern void bound_range_handle(struct interrupt_frame iframe);
extern void invalid_opcode_handler(void);
extern void invalid_opcode_handle(struct interrupt_frame iframe);
extern void device_not_avail_handler(void);
extern void device_not_avail_handle(struct interrupt_frame iframe);
extern void double_fault_handler(void);
extern void double_fault_handle(struct interrupt_frame iframe);
extern void coprocessor_handler(void);
extern void coprocessor_handle(struct interrupt_frame iframe);
extern void invalid_tss_handler(void);
extern void invalid_tss_handle(struct interrupt_frame iframe);
extern void segment_not_present_handler(void);
extern void segment_not_present_handle(struct interrupt_frame iframe);
extern void stack_fault_handler(void);
extern void stack_fault_handle(struct interrupt_frame iframe);
extern void gpf_handler(void);
extern void gpf_handle(struct interrupt_frame iframe);
extern void page_fault_handler(void);
extern void page_fault_handle(uint32_t error_code, struct interrupt_frame iframe);
extern void floating_point_handler(void);
extern void floating_point_handle(struct interrupt_frame iframe);
extern void alignment_check_handler(void);
extern void alignment_check_handle(struct interrupt_frame iframe);
extern void machine_check_handler(void);
extern void machine_check_handle(struct interrupt_frame iframe);
extern void simd_floating_point_handler(void);
extern void simd_floating_point_handle(struct interrupt_frame iframe);
extern void virtualization_handler(void);
extern void virtualization_handle(struct interrupt_frame iframe);
extern void control_protection_handler(void);
extern void control_protection_handle(struct interrupt_frame iframe);
extern void fpu_error_handler(void);
extern void fpu_error_handle(struct interrupt_frame iframe);
extern void pit_handler(void);
extern void pit_handle(struct interrupt_frame iframe);
extern void keyboard_handler(void);
extern void keyboard_handle(void);

#endif