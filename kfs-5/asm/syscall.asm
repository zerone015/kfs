EINTR  equ  4

section .text
extern syscall_dispatch
extern unmasked_signal_pending
extern do_signal
global syscall_handler
global fork_child_trampoline
syscall_handler:
    cld
	push eax
	push ecx
	push edx
	push ebx
	push esi
	push edi
	push ebp
	
	push esp
    call syscall_dispatch			
	add esp, 4

	cmp eax, -EINTR					
	je .syscall_restart			

	mov [esp + 24], eax			
	jmp .check_signal				
.syscall_restart:
	sub dword [esp + 28], 2
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push esp						
	push eax						
	call do_signal				
	add esp, 8
.done:								
	mov edx, [esp + 16]
	mov ecx, [esp + 20]
	mov eax, [esp + 24]
	add esp, 28
    iretd

fork_child_trampoline: 				
    pop edx
    pop ecx
    pop eax
    iretd