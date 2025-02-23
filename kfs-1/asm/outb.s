section .text
global outb
outb:
	mov edx, [esp + 4]
	mov eax, [esp + 8]

	out dx, al
	ret
