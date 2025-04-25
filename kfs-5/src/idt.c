#include "idt.h"
#include "pic.h"
#include "interrupt.h"
#include "syscall.h"

static struct idt_entry idt[IDT_SIZE];

static inline void idt_load(struct idt_ptr *idt_ptr)
{
	__asm__ (
        "lidt (%0)"
        :
        : "r"(idt_ptr)
		: "memory"
    );
}

static void idt_set_gate(int idx, uintptr_t handler, uint8_t type_attr)
{
	idt[idx].offset_low = handler & 0xFFFF;
	idt[idx].selector = KERN_CODE_SEGMENT;
	idt[idx].zero = 0;
	idt[idx].type_attr = type_attr;
	idt[idx].offset_high = handler >> 16;
}

void idt_init(void)
{
	struct idt_ptr idt_ptr;
	
	idt_set_gate(DIVISION_ERR_INT, (uintptr_t)division_error_handler, INT_GATE_PL0);
	idt_set_gate(DEBUG_INT, (uintptr_t)debug_handler, INT_GATE_PL0);
	idt_set_gate(NMI_INT, (uintptr_t)nmi_handler, INT_GATE_PL0);
	idt_set_gate(BREAKPOINT_INT, (uintptr_t)breakpoint_handler, INT_GATE_PL3);
	idt_set_gate(OVERFLOW_INT, (uintptr_t)overflow_handler, INT_GATE_PL0);
	idt_set_gate(BOUND_RANGE_INT, (uintptr_t)bound_range_handler, INT_GATE_PL0);
	idt_set_gate(INVALID_OPCODE_INT, (uintptr_t)invalid_opcode_handler, INT_GATE_PL0);
	idt_set_gate(DEVICE_NOT_AVAIL_INT, (uintptr_t)device_not_avail_handler, INT_GATE_PL0);
	idt_set_gate(DOUBLE_FAULT_INT, (uintptr_t)double_fault_handler, INT_GATE_PL0);
	idt_set_gate(COPROCESSOR_INT, (uintptr_t)coprocessor_handler, INT_GATE_PL0);
	idt_set_gate(INVALID_TSS_INT, (uintptr_t)invalid_tss_handler, INT_GATE_PL0);
	idt_set_gate(SEGMENT_NOT_PRESENT_INT, (uintptr_t)segment_not_present_handler, INT_GATE_PL0);
	idt_set_gate(STACK_FAULT_INT, (uintptr_t)stack_fault_handler, INT_GATE_PL0);
	idt_set_gate(GPF_INT, (uintptr_t)gpf_handler, INT_GATE_PL0);
	idt_set_gate(PAGE_FAULT_INT, (uintptr_t)page_fault_handler, INT_GATE_PL0);
	idt_set_gate(FLOATING_POINT_INT, (uintptr_t)floating_point_handler, INT_GATE_PL0);
	idt_set_gate(ALIGNMENT_CHECK_INT, (uintptr_t)alignment_check_handler, INT_GATE_PL0);
	idt_set_gate(MACHINE_CHECK_INT, (uintptr_t)machine_check_handler, INT_GATE_PL0);
	idt_set_gate(SIMD_FLOATING_POINT_INT, (uintptr_t)simd_floating_point_handler, INT_GATE_PL0);
	idt_set_gate(VIRTUALIZATION_INT, (uintptr_t)virtualization_handler, INT_GATE_PL0);
	idt_set_gate(CONTROL_PROTECTION_INT, (uintptr_t)control_protection_handler, INT_GATE_PL0);
	idt_set_gate(FPU_ERROR_INT, (uintptr_t)fpu_error_handler, INT_GATE_PL0);
	idt_set_gate(PIT_INT, (uintptr_t)pit_handler, INT_GATE_PL0);
	idt_set_gate(KEYBOARD_INT, (uintptr_t)keyboard_handler, INT_GATE_PL0);
	idt_set_gate(SYSCALL_INT, (uintptr_t)syscall_handler, INT_GATE_PL3);
	idt_ptr.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
	idt_ptr.base = (uintptr_t)&idt;
	idt_load(&idt_ptr);
}

