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
division_error_handler:
	cld
	pushad
	call division_error_handle
debug_handler:
	cld
	pushad
	call debug_handle
nmi_handler:
	cld
	pushad
	call nmi_handle
breakpoint_handler:
	cld
	pushad
	call breakpoint_handle
overflow_handler:
	cld
	pushad
	call bound_range_handle
bound_range_handler:
	cld
	pushad
	call bound_range_handle
invalid_opcode_handler:
	cld
	pushad
	call invalid_opcode_handle
device_not_avail_handler:
	cld
	pushad
	call device_not_avail_handle
double_fault_handler:
	cld
	add esp, 4
	pushad
	call double_fault_handle
coprocessor_handler:
	cld
	pushad
	call coprocessor_handle
invalid_tss_handler:
	cld
	add esp, 4
	pushad
	call invalid_tss_handle
segment_not_present_handler:
	cld
	add esp, 4
	pushad
	call segment_not_present_handle
stack_fault_handler:
	cld
	add esp, 4
	pushad
	call stack_fault_handle
gpf_handler:
	cld
	add esp, 4
	pushad
	call gpf_handle
page_fault_handler:
	cld
	xchg eax, [esp]
	sub esp, 32
	mov [esp + 28], ecx
	mov [esp + 24], edx
	mov [esp + 20], ebx
	add esp, 36
	mov [esp - 20], esp
	sub esp, 36
	mov [esp + 12], ebp
	mov [esp + 8], esi
	mov [esp + 4], edi
	mov [esp], eax
	call page_fault_handle
	add esp, 4
	popad
	iretd
floating_point_handler:
	cld
	pushad
	call floating_point_handle
alignment_check_handler:
	cld
	add esp, 4
	pushad
	call alignment_check_handle
machine_check_handler:
	cld
	pushad
	call machine_check_handle
simd_floating_point_handler:
	cld
	pushad
	call simd_floating_point_handle
virtualization_handler:
	cld
	pushad
	call virtualization_handle
control_protection_handler:
	cld
	add esp, 4
	pushad
	call control_protection_handle
fpu_error_handler:
	cld
	pushad
	call fpu_error_handle
pit_handler:
	pushad
	call pit_handle
	popad
	iretd
keyboard_handler:
	cld
	pushad
	call keyboard_handle
	popad
	iretd