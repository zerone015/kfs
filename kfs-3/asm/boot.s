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
    resb 16384
stack_top:

section .bss nobits
align 4096
boot_page_directory:
    resb 4096
boot_page_table1:
    resb 4096
boot_page_table2:
    resb 4096
section .multiboot.text
extern _kernel_start
extern _kernel_end
global _start
_start:
    mov edi, boot_page_table1 - 0xC0000000
    mov esi, 0
    mov ecx, 1024

.loop_page_mapping:
    cmp esi, _kernel_start
    jl .skip_mapping
    cmp esi, _kernel_end
    jge .mbd_vga_mapping

    mov edx, esi
    or edx, 0x103
    mov [edi], edx

.skip_mapping:
    add esi, 4096
    add edi, 4
    loop .loop_page_mapping

.mbd_vga_mapping:
    mov edx, ebx 
    and edx, 0xFFFFF000
    or edx, 0x003
    mov [edi], edx
    and ebx, 0x00000FFF
    mov edx, ecx
    sub edx, 1024
    neg edx
    shl edx, 12
    or edx, 0xC0000000
    or ebx, edx

    mov dword [edi + 4], 0x000B8003
    mov edx, ecx
    sub edx, 1025
    neg edx
    shl edx, 12
    or edx, 0xC0000000

.page_table_mapping:
    mov dword [boot_page_directory - 0xC0000000], boot_page_table1 - 0xC0000000 + 0x003
    mov dword [boot_page_directory - 0xC0000000 + 768 * 4], boot_page_table1 - 0xC0000000 + 0x003
    mov dword [boot_page_directory - 0xC0000000 + 769 * 4], boot_page_table2 - 0xC0000000 + 0x003

.recursive_paging:
    mov dword [boot_page_directory - 0xC0000000 + 1023 * 4], boot_page_directory - 0xC0000000 + 0x003

.enable_paging:
    mov ecx, boot_page_directory - 0xC0000000
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    mov ecx, cr4
    or ecx, 0x00000080
    mov cr4, ecx

    jmp .higher_half

section .text
extern kmain
.higher_half:
    mov dword [boot_page_directory], 0

    mov ecx, cr3
    mov cr3, ecx

    mov esp, stack_top
    sub esp, 16
    mov [esp], ebx
    mov [esp + 4], eax
    mov [esp + 8], edx
    
    call kmain
    sti
.halt:
    hlt
    jmp .halt
