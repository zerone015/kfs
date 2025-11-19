section .text
global isr_signal_return

extern unmasked_signal_pending
extern do_signal

; Common return path for exceptions/IRQs
isr_signal_return:
    call unmasked_signal_pending
    cmp eax, 0
    je .done

    push ebx
    push esi
    push edi
    push ebp
    push esp
    push eax
    call do_signal
    add esp, 24

.done:
    pop edx
    pop ecx
    pop eax
    iretd

section .text
global syscall_signal_return

extern unmasked_signal_pending
extern do_signal

; Return path for system calls (INT 0x80)
syscall_signal_return:
    call unmasked_signal_pending
    cmp eax, 0
    je .done

    push esp      
    push eax
    call do_signal
    add esp, 8

.done:
    mov edx, [esp + 16]
    mov ecx, [esp + 20]
    mov eax, [esp + 24]
    add esp, 28
    iretd