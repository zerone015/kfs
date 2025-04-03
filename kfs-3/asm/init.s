section .text
global gdt_load
global idt_load
gdt_load:
	mov eax, [esp + 4]
	lgdt [eax]
.gdt_reload_selector:
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
idt_load:
	mov edx, [esp + 4]
	lidt [edx]
	ret
