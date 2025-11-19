%include "offsets.inc"

section .text
extern syscall_dispatch
extern unmasked_signal_pending
extern syscall_signal_return
extern do_exit
extern current

global syscall_handler
global fork_child_trampoline
global sys_sigreturn


; ----------------------------------------------------------------------
; System call entry (INT 0x80)
; ----------------------------------------------------------------------
syscall_handler:
    cld
    push eax
    push ecx
    push edx
    push ebx
    push esi
    push edi
    push ebp

    push esp                     ; struct ucontext*
    call syscall_dispatch
    add esp, 4

    cmp eax, -EINTR
    je .syscall_restart

    mov [esp + 24], eax          ; return value
    jmp .signal_return

.syscall_restart:
    sub dword [esp + 28], 2      ; regs->eip -= 2

.signal_return:
    jmp syscall_signal_return


; ----------------------------------------------------------------------
; Child return path after fork()
; ----------------------------------------------------------------------
fork_child_trampoline:
    pop edx
    pop ecx
    pop eax
    iretd


; ----------------------------------------------------------------------
; sigreturn() - restore user context after signal handler
; ----------------------------------------------------------------------
sys_sigreturn:
    mov eax, [esp + 4]                   ; struct ucontext*
    mov ebx, [eax + OFFSET_UCONTEXT_ESP]
    sub ebx, OFFSET_SIGFRAME_ARG         ; struct signal_frame*

    cmp dword [ebx + OFFSET_SIGFRAME_EIP], 0xC0000000
    jae .do_exit

    mov edx, [current]
    mov byte [edx + OFFSET_TASK_CURRENT_SIG], 0

    push dword [eax + OFFSET_UCONTEXT_SS]
    push dword [ebx + OFFSET_SIGFRAME_ESP]

    mov edx, [eax + OFFSET_UCONTEXT_EFLAGS]
    mov ecx, [ebx + OFFSET_SIGFRAME_EFLAGS]
    and edx, ~EFLAGS_USER_MASK
    and ecx, EFLAGS_USER_MASK
    or  edx, ecx
    push edx

    push dword [eax + OFFSET_UCONTEXT_CS]
    push dword [ebx + OFFSET_SIGFRAME_EIP]

    mov eax, [ebx + OFFSET_SIGFRAME_EAX]
    mov ecx, [ebx + OFFSET_SIGFRAME_ECX]
    mov edx, [ebx + OFFSET_SIGFRAME_EDX]
    mov esi, [ebx + OFFSET_SIGFRAME_ESI]
    mov edi, [ebx + OFFSET_SIGFRAME_EDI]
    mov ebp, [ebx + OFFSET_SIGFRAME_EBP]
    mov ebx, [ebx + OFFSET_SIGFRAME_EBX]

    iretd

.do_exit:
    push SIGSEGV
    call do_exit