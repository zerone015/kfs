MBALIGN  equ  1 << 0            
MEMINFO  equ  1 << 1            
MBFLAGS  equ  MBALIGN | MEMINFO 
MAGIC    equ  0x1BADB002        
CHECKSUM equ -(MAGIC + MBFLAGS) 

section .multiboot.data
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

section .bootstrap_stack nobits
align 16
stack_bottom:
    resb 8192
stack_top:

section .bss nobits
align 4096
boot_page_directory:
    resb 4096
section .multiboot.text
global _start
_start:
    mov dword [boot_page_directory - 0xC0000000], 0x0 + 0x083
    mov dword [boot_page_directory - 0xC0000000 + 1 * 4], 0x400000 + 0x083
    mov dword [boot_page_directory - 0xC0000000 + 768 * 4], 0x0 + 0x183
    mov dword [boot_page_directory - 0xC0000000 + 769 * 4], 0x400000 + 0x183

    mov esi, ebx
    and esi, 0xFFC00000
    or esi, 0x083
    mov [boot_page_directory - 0xC0000000 + 770 * 4], esi
    and ebx, 0x003FFFFF
    or ebx, 0xC0800000

    mov dword [boot_page_directory - 0xC0000000 + 1023 * 4], boot_page_directory - 0xC0000000 + 0x003

    mov ecx, boot_page_directory - 0xC0000000
    mov cr3, ecx

    mov ecx, cr4
    or ecx, 0x00000090
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    jmp .higher_half

section .text
extern kmain
.higher_half:
    mov dword [boot_page_directory], 0
    mov dword [boot_page_directory + 1 * 4], 0

    mov ecx, cr3
    mov cr3, ecx

    mov esp, stack_top
    sub esp, 16
    mov [esp], ebx
    mov [esp + 4], eax
    
    call kmain
    sti
.halt:
    hlt
    jmp .halt
