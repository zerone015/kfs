section .text
extern syscall_dispatch
global syscall_handler
global child_return_address
syscall_handler:
    cld
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
    call syscall_dispatch
	mov edx, [esp + 12]
	mov ecx, [esp + 8]
	add esp, 28
    iretd
child_return_address:
    pop eax
    pop ecx
    pop edx
    iretd