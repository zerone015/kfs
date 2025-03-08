#include "idt.h"
#include "keyboard_handle.h"
#include "pic.h"

static struct idt_entry idt[IDT_SIZE];

void idt_init(void)
{
	struct idt_ptr idt_ptr;

	idt_set_gate(KEYBOARD_INT, (uint32_t)keyboard_handler);
	idt_ptr.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
	idt_ptr.base = (uint32_t)&idt;
	idt_load((uint32_t)&idt_ptr);
}

void idt_set_gate(int idx, uint32_t handler)
{
	idt[idx].offset_low = handler & 0xFFFF;
	idt[idx].selector = KERN_CODE_SEGMENT;
	idt[idx].zero = 0;
	idt[idx].type_attr = INT_GATE;
	idt[idx].offset_high = handler >> 16;
}
