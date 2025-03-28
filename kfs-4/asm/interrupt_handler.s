section .text
global keyboard_handler
extern keyboard_handle
global page_fault_handler
extern page_fault_handle
global division_error_handler
extern division_error_handle
keyboard_handler:
	cld
	pushad
	call keyboard_handle
	popad
	iretd
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
division_error_handler:
	cld
	pushad
	call division_error_handle
	popad
	iretd