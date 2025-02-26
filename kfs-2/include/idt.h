#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>
#include "pic.h"

#define IDT_SIZE		256
#define KERN_CODE_SEGMENT	0x08
#define INT_GATE		0x8E
#define TRAP_GATE		0x8F
#define TASK_GATE		0x85
#define KEYBOARD_INT		(PIC1_OFFSET + 1)

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_init(void);
void idt_load(uint32_t idt_addr);
void idt_set_gate(int idx, uint32_t handler);

#endif
