GDT_SELECTOR_CODE_PL0  equ  0x08

section .rodata
	 panic_msg db "fatal exception", 0
section .text
extern panic
extern unmasked_signal_pending
extern do_signal
extern division_error_handle
extern debug_handle
extern invalid_opcode_handle
extern device_not_avail_handle
extern double_fault_handle
extern invalid_tss_handle
extern segment_not_present_handle
extern stack_fault_handle
extern gpf_handle
extern page_fault_handle
extern floating_point_handle
extern alignment_check_handle
extern machine_check_handle
extern simd_floating_point_handle
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
global invalid_tss_handler
global segment_not_present_handler
global stack_fault_handler
global gpf_handler
global page_fault_handler
global floating_point_handler
global alignment_check_handler
global machine_check_handler
global simd_floating_point_handler
global pit_handler
global keyboard_handler
isr_panic:
    push dword [esp + 16]           ; eflags
    push dword [esp + 8]            ; eip
    pushad
    add dword [esp + 12], 28        ; esp
    push esp                        ; panic_info
    push dword [esp + 48]           ; panic message
    call panic
division_error_handler:
	cld
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .division_error_handle
	push panic_msg
	call isr_panic
.division_error_handle:
	push eax
	push ecx
	push edx
	call division_error_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
debug_handler:
	cld
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .debug_handle
	push panic_msg
	call isr_panic
.debug_handle:
	push eax
	push ecx
	push edx
	call debug_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done	
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
nmi_handler:
	cld
	push panic_msg
	call isr_panic
breakpoint_handler:
	cld
	push panic_msg
	call isr_panic
overflow_handler:
	cld
	push panic_msg
	call isr_panic
bound_range_handler:
	cld
	push panic_msg
	call isr_panic
invalid_opcode_handler:
	cld
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .invalid_opcode_handle
	push panic_msg
	call isr_panic
.invalid_opcode_handle:
	push eax
	push ecx
	push edx
	call invalid_opcode_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
device_not_avail_handler:
	cld
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .device_not_avail_handle
	push panic_msg
	call isr_panic
.device_not_avail_handle:
	push eax
	push ecx
	push edx
	call device_not_avail_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
double_fault_handler:
	cld
	add esp, 4
	push panic_msg
	call isr_panic
invalid_tss_handler:
	cld
	add esp, 4
	push panic_msg
	call isr_panic
segment_not_present_handler:
	cld
	add esp, 4
	push panic_msg
	call isr_panic
stack_fault_handler:
	cld
	add esp, 4
	push panic_msg
	call isr_panic
gpf_handler:
	cld
	add esp, 4
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .gpf_handle
	push panic_msg
	call isr_panic
.gpf_handle:
	push eax
	push ecx
	push edx
	call gpf_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
page_fault_handler:
	cld
	push ecx
	push edx
	mov edx, [esp + 8]
	mov [esp + 8], eax
	push edx
	mov edx, cr2
	push edx
	call page_fault_handle
	add esp, 8
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done	
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
floating_point_handler:
	cld
	push panic_msg
	call isr_panic
alignment_check_handler:
	cld
	add esp, 4
	push panic_msg
	call isr_panic
machine_check_handler:
	cld
	push panic_msg
	call isr_panic
simd_floating_point_handler:
	cld
	push panic_msg
	call isr_panic
pit_handler:
	cld
	push eax
	push ecx
	push edx
	call pit_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd
keyboard_handler:
	cld
	push eax
	push ecx
	push edx
	call keyboard_handle
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push ebx
	push esi
	push edi
	push ebp
	push esp						
	push eax						
	call do_signal				
	add esp, 24
.done:
	pop edx
	pop ecx
	pop eax
	iretd