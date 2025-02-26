section .text
global gdt_load
gdt_load:
	mov edx, [esp + 4]
	lgdt [edx]
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
