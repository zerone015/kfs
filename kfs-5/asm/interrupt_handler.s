section .text
extern division_error_handle
extern debug_handle
extern nmi_handle
extern breakpoint_handle
extern overflow_handle
extern bound_range_handle
extern invalid_opcode_handle
extern device_not_avail_handle
extern double_fault_handle
extern coprocessor_handle
extern invalid_tss_handle
extern segment_not_present_handle
extern stack_fault_handle
extern gpf_handle
extern page_fault_handle
extern floating_point_handle
extern alignment_check_handle
extern machine_check_handle
extern simd_floating_point_handle
extern virtualization_handle
extern control_protection_handle
extern fpu_error_handle
extern pit_handle
extern keyboard_handle
extern syscall_handle
global division_error_handler
global debug_handler
global nmi_handler
global breakpoint_handler
global overflow_handler
global bound_range_handler
global invalid_opcode_handler
global device_not_avail_handler
global double_fault_handler
global coprocessor_handler
global invalid_tss_handler
global segment_not_present_handler
global stack_fault_handler
global gpf_handler
global page_fault_handler
global floating_point_handler
global alignment_check_handler
global machine_check_handler
global simd_floating_point_handler
global virtualization_handler
global control_protection_handler
global fpu_error_handler
global pit_handler
global keyboard_handler
global syscall_handler	
division_error_handler:
	cld
	push 0
	pushad
	jmp division_error_handle
debug_handler:
	cld
	push 0
	pushad
	jmp debug_handle
nmi_handler:
	cld
	push 0
	pushad
	jmp nmi_handle
breakpoint_handler:
	cld
	push 0
	pushad
	jmp breakpoint_handle
overflow_handler:
	cld
	pushad
	jmp bound_range_handle
bound_range_handler:
	cld
	push 0
	pushad
	jmp bound_range_handle
invalid_opcode_handler:
	cld
	push 0
	pushad
	jmp invalid_opcode_handle
device_not_avail_handler:
	cld
	push 0
	pushad
	jmp device_not_avail_handle
double_fault_handler:
	cld
	pushad
	jmp double_fault_handle
coprocessor_handler:
	cld
	push 0
	pushad
	jmp coprocessor_handle
invalid_tss_handler:
	cld
	pushad
	jmp invalid_tss_handle
segment_not_present_handler:
	cld
	pushad
	jmp segment_not_present_handle
stack_fault_handler:
	cld
	pushad
	jmp stack_fault_handle
gpf_handler:
	cld
	pushad
	jmp gpf_handle
page_fault_handler:
	cld
	pushad
	call page_fault_handle
	popad
	add esp, 4
	iretd
floating_point_handler:
	cld
	push 0
	pushad
	jmp floating_point_handle
alignment_check_handler:
	cld
	pushad
	jmp alignment_check_handle
machine_check_handler:
	cld
	push 0
	pushad
	jmp machine_check_handle
simd_floating_point_handler:
	cld
	push 0
	pushad
	jmp simd_floating_point_handle
virtualization_handler:
	cld
	push 0
	pushad
	jmp virtualization_handle
control_protection_handler:
	cld
	pushad
	jmp control_protection_handle
fpu_error_handler:
	cld
	push 0
	pushad
	jmp fpu_error_handle
pit_handler:
	push 0
	pushad
	mov ax, 0x10
	mov ds, ax
    mov es, ax
	jmp pit_handle
keyboard_handler:
	cld
	pushad
	call keyboard_handle
	popad
	iretd
syscall_handler:
	push 0
	pushad
    push ds
    push es
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    call syscall_handle
    pop es
    pop ds
    popad
	add esp, 4
    iretd