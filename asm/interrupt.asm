GDT_SELECTOR_CODE_PL0  equ  0x08

section .rodata
	 panic_msg db "fatal exception", 0
	 de_panic_msg db "division error", 0
	 db_panic_msg db "debug exception", 0
	 nmi_panic_msg db "fatal hardware error", 0
	 bp_panic_msg db "breakpoint exception", 0
	 of_panic_msg db "overflow exception", 0
	 br_panic_msg db "bound range exceeded", 0
	 ud_panic_msg db "invalid opcode", 0
	 nm_panic_msg db "devide not available", 0
	 df_panic_msg db "double fault", 0
	 ts_panic_msg db "invalid tss", 0
	 np_panic_msg db "segment not present", 0
	 ss_panic_msg db "stack segment fault", 0
	 gp_panic_msg db "general protection fault", 0
	 mf_panic_msg db "floating-point exception", 0
	 ac_panic_msg db "alignment check", 0
	 mc_panic_msg db "machine check", 0
	 xm_panic_msg db "simd floating-point exception", 0
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
extern ata_handle
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
global primary_ata_handler
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
	push de_panic_msg
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
	push db_panic_msg
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
	push nmi_panic_msg
	call isr_panic
breakpoint_handler:
	cld
	push bp_panic_msg 
	call isr_panic
overflow_handler:
	cld
	push of_panic_msg
	call isr_panic
bound_range_handler:
	cld
	push br_panic_msg
	call isr_panic
invalid_opcode_handler:
	cld
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .invalid_opcode_handle
	push ud_panic_msg
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
	push nm_panic_msg
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
	push df_panic_msg
	call isr_panic
invalid_tss_handler:
	cld
	add esp, 4
	push ts_panic_msg
	call isr_panic
segment_not_present_handler:
	cld
	add esp, 4
	push np_panic_msg
	call isr_panic
stack_fault_handler:
	cld
	add esp, 4
	push ss_panic_msg
	call isr_panic
gpf_handler:
	cld
	add esp, 4
	cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
	jne .gpf_handle
	push gp_panic_msg
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
	push mf_panic_msg
	call isr_panic
alignment_check_handler:
	cld
	add esp, 4
	push ac_panic_msg
	call isr_panic
machine_check_handler:
	cld
	push mc_panic_msg
	call isr_panic
simd_floating_point_handler:
	cld
	push xm_panic_msg
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
primary_ata_handler:
	cld
	push eax
	push ecx
	push edx
	call ata_handle
.done:
	pop edx
	pop ecx
	pop eax
	iretd