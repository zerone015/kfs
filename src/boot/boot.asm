KERNEL_BASE        equ 0xC0000000

MEMINFO            equ (1 << 1)           ; request memory info from GRUB
MBFLAGS            equ MEMINFO
MAGIC              equ 0x1BADB002         ; multiboot magic value
CHECKSUM           equ -(MAGIC + MBFLAGS) ; magic + flags + checksum = 0

PDE_PRESENT        equ (1 << 0)           ; entry is present
PDE_RW             equ (1 << 1)           ; writable
PDE_PS             equ (1 << 7)           ; 4MiB page
PDE_GLOBAL         equ (1 << 8)           ; global mapping

PDE_4MB_KERNEL     equ (PDE_PRESENT | PDE_RW | PDE_PS)              ; 0x083
PDE_4MB_GLOBAL     equ (PDE_PRESENT | PDE_RW | PDE_PS | PDE_GLOBAL) ; 0x183
PDE_PT_KERNEL      equ (PDE_PRESENT | PDE_RW)                       ; 0x003

CR4_PSE            equ (1 << 4)                     ; enable 4MiB pages
CR4_PGE            equ (1 << 7)                     ; enable global pages
CR4_BOOT_FLAGS     equ (CR4_PSE | CR4_PGE)          ; = 0x00000090

CR0_PG             equ 0x80000000                   ; enable paging
CR0_WP             equ 0x00010000                   ; write-protect for supervisor accesses
CR0_NE             equ 0x00000020                   ; x87 FP exceptions use #MF instead of IRQ13
CR0_BOOT_FLAGS     equ (CR0_PG | CR0_WP | CR0_NE)   ; = 0x80010020

section .multiboot.data
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

section .bootstrap_stack nobits
align 4
global stack_top
stack_bottom:
    resb 8192
stack_top:

section .bss nobits
align 4096
boot_page_directory:
    resb 4096
boot_page_table:
    resb 4096

section .multiboot.text
global _start
_start:

    ; Identity map 0–8MiB
    mov dword [boot_page_directory - KERNEL_BASE + 0*4], (0x00000000 | PDE_4MB_KERNEL)
    mov dword [boot_page_directory - KERNEL_BASE + 1*4], (0x00400000 | PDE_4MB_KERNEL)

    ; Higher-half mapping at 0xC0000000
    mov dword [boot_page_directory - KERNEL_BASE + 768*4], (0x00000000 | PDE_4MB_GLOBAL)
    mov dword [boot_page_directory - KERNEL_BASE + 769*4], (0x00400000 | PDE_4MB_GLOBAL)

    ; Map multiboot info (EBX physical)
    mov esi, ebx
    and esi, 0xFFC00000
    or  esi, PDE_4MB_KERNEL
    mov [boot_page_directory - KERNEL_BASE + 770*4], esi

    ; Convert EBX to higher-half address
    and ebx, 0x003FFFFF
    or  ebx, 0xC0800000

    ; Temporary 4KiB mapping region (used to map any PT for manipulation)
    mov dword [boot_page_directory - KERNEL_BASE + 1022*4], \
        boot_page_table - KERNEL_BASE + PDE_PT_KERNEL

    ; Recursive mapping
    mov dword [boot_page_directory - KERNEL_BASE + 1023*4], \
        boot_page_directory - KERNEL_BASE + PDE_PT_KERNEL

    ; Load CR3
    mov ecx, boot_page_directory - KERNEL_BASE
    mov cr3, ecx

    ; Enable PSE + global pages
    mov ecx, cr4
    or  ecx, CR4_BOOT_FLAGS
    mov cr4, ecx

    ; Enable paging + WP + NE
    mov ecx, cr0
    or  ecx, CR0_BOOT_FLAGS
    mov cr0, ecx

    jmp .higher_half

section .text
extern kmain
.higher_half:
    ; Remove identity mappings
    mov dword [boot_page_directory + 0*4], 0
    mov dword [boot_page_directory + 1*4], 0

    ; TLB flush
    mov ecx, cr3
    mov cr3, ecx

    ; Switch to kernel stack
    mov esp, stack_top

    ; Call kmain(magic, multiboot_info)
    push eax
    push ebx
    call kmain

.halt:
    hlt
    jmp .halt
