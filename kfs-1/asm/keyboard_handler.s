section .text
global keyboard_handler
extern keyboard_handle
keyboard_handler:
	pushad
	cld
	call keyboard_handle
	popad
	iretd
