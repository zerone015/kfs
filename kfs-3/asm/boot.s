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
boot_page_table1:
    resb 4096
boot_page_table2:
    resb 4096
section .multiboot.text
extern _kernel_start
extern _kernel_end
extern _kernel_text_start
extern _kernel_data_start
global _start
_start:
    mov edi, boot_page_table1 - 0xC0000000
    mov esi, 0
    mov ecx, 1024 * 2 - 2

.loop_page_mapping:
    cmp esi, _kernel_start
    jl .increase_offset
    cmp esi, _kernel_end
    jge .vga_mbd_page_mapping

    mov edx, esi

    cmp edx, _kernel_text_start
    jl .rdwr_mode
    cmp edx, _kernel_data_start
    jge .rdwr_mode

.readonly_mode:
    or edx, 0x101
    jmp .register_entry

.rdwr_mode:
    or edx, 0x103

.register_entry:
    mov [edi], edx

.increase_offset:
    add esi, 4096
    add edi, 4
    loop .loop_page_mapping

.vga_mbd_page_mapping:
    mov dword [edi], 0x000B8000 + 0x103
    mov edx, ecx
    sub edx, 1024 * 2 - 2
    neg edx
    shl edx, 12
    or edx, 0xC0000000

    mov esi, ebx 
    and esi, 0xFFFFF000
    or esi, 0x003
    mov [edi + 4], esi
    and ebx, 0x00000FFF
    mov esi, ecx
    sub esi, 1024 * 2 - 1
    neg esi
    shl esi, 12
    or esi, 0xC0000000
    or ebx, esi

.page_table_mapping:
    mov dword [boot_page_directory - 0xC0000000], boot_page_table1 - 0xC0000000 + 0x003
    mov dword [boot_page_directory - 0xC0000000 + 768 * 4], boot_page_table1 - 0xC0000000 + 0x003
    mov dword [boot_page_directory - 0xC0000000 + 769 * 4], boot_page_table2 - 0xC0000000 + 0x003

.recursive_mapping:
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
