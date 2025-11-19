GDT_SELECTOR_CODE_PL0  equ 0x08

section .rodata
    de_panic_msg db "division error", 0
    db_panic_msg db "debug exception", 0
    nmi_panic_msg db "fatal hardware error", 0
    bp_panic_msg db "breakpoint exception", 0
    of_panic_msg db "overflow exception", 0
    br_panic_msg db "bound range exceeded", 0
    ud_panic_msg db "invalid opcode", 0
    nm_panic_msg db "devide not available", 0
    mf_panic_msg db "floating-point exception", 0
    ac_panic_msg db "alignment check", 0
    mc_panic_msg db "machine check", 0
    xm_panic_msg db "simd floating-point exception", 0
    df_panic_msg db "double fault", 0
    ts_panic_msg db "invalid tss", 0
    np_panic_msg db "segment not present", 0
    ss_panic_msg db "stack segment fault", 0
    gp_panic_msg db "general protection fault", 0

section .text
extern isr_panic
extern isr_signal_return

extern division_error_handle
extern debug_handle
extern invalid_opcode_handle
extern device_not_avail_handle
extern floating_point_handle
extern alignment_check_handle
extern machine_check_handle
extern simd_floating_point_handle

extern gpf_handle
extern page_fault_handle
extern invalid_tss_handle
extern segment_not_present_handle
extern stack_fault_handle

extern pit_handle
extern keyboard_handle


; ============================================================
; ===================== CPU EXCEPTIONS =======================
; ============================================================

; ------------------------------
; #DE Divide Error
; ------------------------------
global division_error_handler
division_error_handler:
    cld
    cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
    jne .dispatch
    push de_panic_msg
    call isr_panic
.dispatch:
    push eax
    push ecx
    push edx
    call division_error_handle
    jmp isr_signal_return


; ------------------------------
; #DB Debug Exception
; ------------------------------
global debug_handler
debug_handler:
    cld
    cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
    jne .dispatch
    push db_panic_msg
    call isr_panic
.dispatch:
    push eax
    push ecx
    push edx
    call debug_handle
    jmp isr_signal_return


; ------------------------------
; #02 NMI
; ------------------------------
global nmi_handler
nmi_handler:
    cld
    push nmi_panic_msg
    call isr_panic


; ------------------------------
; #BP Breakpoint
; ------------------------------
global breakpoint_handler
breakpoint_handler:
    cld
    push bp_panic_msg
    call isr_panic


; ------------------------------
; #OF Overflow
; ------------------------------
global overflow_handler
overflow_handler:
    cld
    push of_panic_msg
    call isr_panic


; ------------------------------
; #BR Bound Range Exceeded
; ------------------------------
global bound_range_handler
bound_range_handler:
    cld
    push br_panic_msg
    call isr_panic


; ------------------------------
; #UD Invalid Opcode
; ------------------------------
global invalid_opcode_handler
invalid_opcode_handler:
    cld
    cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
    jne .dispatch
    push ud_panic_msg
    call isr_panic
.dispatch:
    push eax
    push ecx
    push edx
    call invalid_opcode_handle
    jmp isr_signal_return


; ------------------------------
; #NM Device Not Available
; ------------------------------
global device_not_avail_handler
device_not_avail_handler:
    cld
    cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
    jne .dispatch
    push nm_panic_msg
    call isr_panic
.dispatch:
    push eax
    push ecx
    push edx
    call device_not_avail_handle
    jmp isr_signal_return


; ------------------------------
; #MF FP Error
; ------------------------------
global floating_point_handler
floating_point_handler:
    cld
    push mf_panic_msg
    call isr_panic


; ------------------------------
; #AC Alignment Check
; ------------------------------
global alignment_check_handler
alignment_check_handler:
    cld
    add esp, 4
    push ac_panic_msg
    call isr_panic


; ------------------------------
; #MC Machine Check
; ------------------------------
global machine_check_handler
machine_check_handler:
    cld
    push mc_panic_msg
    call isr_panic


; ------------------------------
; #XM SIMD FP
; ------------------------------
global simd_floating_point_handler
simd_floating_point_handler:
    cld
    push xm_panic_msg
    call isr_panic


; ------------------------------
; #DF Double Fault
; ------------------------------
global double_fault_handler
double_fault_handler:
    cld
    add esp, 4
    push df_panic_msg
    call isr_panic


; ------------------------------
; #TS Invalid TSS
; ------------------------------
global invalid_tss_handler
invalid_tss_handler:
    cld
    add esp, 4
    push ts_panic_msg
    call isr_panic


; ------------------------------
; #NP Segment Not Present
; ------------------------------
global segment_not_present_handler
segment_not_present_handler:
    cld
    add esp, 4
    push np_panic_msg
    call isr_panic


; ------------------------------
; #SS Stack Fault
; ------------------------------
global stack_fault_handler
stack_fault_handler:
    cld
    add esp, 4
    push ss_panic_msg
    call isr_panic


; ------------------------------
; #GP General Protection Fault
; ------------------------------
global gpf_handler
gpf_handler:
    cld
    add esp, 4
    cmp dword [esp + 4], GDT_SELECTOR_CODE_PL0
    jne .dispatch
    push gp_panic_msg
    call isr_panic
.dispatch:
    push eax
    push ecx
    push edx
    call gpf_handle
    jmp isr_signal_return


; ------------------------------
; #PF Page Fault
; ------------------------------
global page_fault_handler
page_fault_handler:
    cld
    push ecx
    push edx
    mov edx, [esp + 8]        ; error code
    mov [esp + 8], eax
    push edx
    mov edx, cr2
    push edx
    call page_fault_handle
    add esp, 8
    jmp isr_signal_return


; ============================================================
; ========================= IRQs =============================
; ============================================================

; ------------------------------
; PIT IRQ0
; ------------------------------
global pit_handler
pit_handler:
    cld
    push eax
    push ecx
    push edx
    call pit_handle
    jmp isr_signal_return


; ------------------------------
; Keyboard IRQ1
; ------------------------------
global keyboard_handler
keyboard_handler:
    cld
    push eax
    push ecx
    push edx
    call keyboard_handle
    jmp isr_signal_return
