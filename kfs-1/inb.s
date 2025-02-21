section .text
global inb
inb:
	mov edx, [esp + 4]
	in al, dx
	ret
