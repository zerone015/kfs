section .text
global page_fault_handler
extern page_fault_handle
page_fault_handler:
	xchg eax, [esp]
	pushad
	push eax
	call page_fault_handle
	add esp, 4
	popad
	pop eax
	iretd