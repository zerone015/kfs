#include "idt.h"
#include "pic.h"
#include "interrupt.h"

static struct idt_entry idt[IDT_SIZE];

void idt_init(void)
{
	struct idt_ptr idt_ptr;
	
	idt_set_gate(DIVISION_ERR_INT, (uint32_t)division_error_handler);
	idt_set_gate(DEBUG_INT, (uint32_t)debug_handler);
	idt_set_gate(NMI_INT, (uint32_t)nmi_handler);
	idt_set_gate(BREAKPOINT_INT, (uint32_t)breakpoint_handler);
	idt_set_gate(OVERFLOW_INT, (uint32_t)overflow_handler);
	idt_set_gate(BOUND_RANGE_INT, (uint32_t)bound_range_handler);
	idt_set_gate(INVALID_OPCODE_INT, (uint32_t)invalid_opcode_handler);
	idt_set_gate(DEVICE_NOT_AVAIL_INT, (uint32_t)device_not_avail_handler);
	idt_set_gate(DOUBLE_FAULT_INT, (uint32_t)double_fault_handler);
	idt_set_gate(COPROCESSOR_INT, (uint32_t)coprocessor_handler);
	idt_set_gate(INVALID_TSS_INT, (uint32_t)invalid_tss_handler);
	idt_set_gate(SEGMENT_NOT_PRESENT_INT, (uint32_t)segment_not_present_handler);
	idt_set_gate(STACK_FAULT_INT, (uint32_t)stack_fault_handler);
	idt_set_gate(GPF_INT, (uint32_t)gpf_handler);
	idt_set_gate(PAGE_FAULT_INT, (uint32_t)page_fault_handler);
	idt_set_gate(FLOATING_POINT_INT, (uint32_t)floating_point_handler);
	idt_set_gate(ALIGNMENT_CHECK_INT, (uint32_t)alignment_check_handler);
	idt_set_gate(MACHINE_CHECK_INT, (uint32_t)machine_check_handler);
	idt_set_gate(SIMD_FLOATING_POINT_INT, (uint32_t)simd_floating_point_handler);
	idt_set_gate(VIRTUALIZATION_INT, (uint32_t)virtualization_handler);
	idt_set_gate(CONTROL_PROTECTION_INT, (uint32_t)control_protection_handler);
	idt_set_gate(FPU_ERROR_INT, (uint32_t)fpu_error_handler);
	idt_set_gate(PIT_INT, (uint32_t)pit_handler);
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