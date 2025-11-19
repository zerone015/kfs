; Panic entry stubs.
; do_panic  : invoked from normal kernel context.
; isr_panic : invoked from ISR/exception context.
; They are separated because both contexts have different stack layouts,
; and panic() expects a unified panic_info frame.

section .text
global do_panic
extern panic

do_panic:
    pushfd                  ; EFLAGS
    push dword [esp + 4]    ; EIP
    pushad
    add dword [esp + 12], 16
    push esp                ; panic_info
    push dword [esp + 48]   ; msg
    call panic


section .text
global isr_panic
extern panic

isr_panic:
    push dword [esp + 16]   ; EFLAGS
    push dword [esp + 8]    ; EIP
    pushad
    add dword [esp + 12], 28
    push esp                ; panic_info
    push dword [esp + 48]   ; msg
    call panic