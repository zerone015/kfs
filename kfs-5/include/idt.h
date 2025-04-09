#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>
#include "pic.h"

#define IDT_SIZE		        256
#define KERN_CODE_SEGMENT	    0x08
#define INT_GATE_PL0		    0x8E
#define INT_GATE_PL3		    0xEE
#define TRAP_GATE		        0x8F
#define TASK_GATE		        0x85
#define DIVISION_ERR_INT        0x0
#define DEBUG_INT               0x1
#define NMI_INT                 0x2
#define BREAKPOINT_INT          0x3
#define OVERFLOW_INT            0x4
#define BOUND_RANGE_INT         0x5
#define INVALID_OPCODE_INT      0x6
#define DEVICE_NOT_AVAIL_INT    0x7
#define DOUBLE_FAULT_INT        0x8
#define COPROCESSOR_INT         0x9
#define INVALID_TSS_INT         0xA
#define SEGMENT_NOT_PRESENT_INT 0xB
#define STACK_FAULT_INT			0xC
#define GPF_INT					0xD
#define PAGE_FAULT_INT          0xE
#define FLOATING_POINT_INT		0x10
#define ALIGNMENT_CHECK_INT		0x11
#define MACHINE_CHECK_INT		0x12
#define SIMD_FLOATING_POINT_INT	0x13
#define VIRTUALIZATION_INT		0x14
#define CONTROL_PROTECTION_INT	0x15
#define FPU_ERROR_INT           (PIC1_OFFSET + 13)
#define PIT_INT                 PIC1_OFFSET
#define KEYBOARD_INT		    (PIC1_OFFSET + 1)
#define SYSCALL_INT             0x80

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed, aligned(8)));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_init(void);

#endif
