%include "offsets.inc"

section .text
extern syscall_dispatch
extern unmasked_signal_pending
extern do_signal
extern do_exit
extern current
global syscall_handler
global fork_child_trampoline
global sys_sigreturn
syscall_handler:
    cld
	push eax
	push ecx
	push edx
	push ebx
	push esi
	push edi
	push ebp
	
	push esp											; struct ucontext *ucontext
    call syscall_dispatch			
	add esp, 4

	cmp eax, -EINTR					
	je .syscall_restart			

	mov [esp + 24], eax									; change [syscall number -> syscall return value]
	jmp .check_signal				
.syscall_restart:
	sub dword [esp + 28], 2								; regs->eip -= 2
.check_signal:
	call unmasked_signal_pending	
	cmp eax, 0						
	je .done				
.do_signal:
	push esp											; struct ucontext *ucontext
	push eax											; int signal_pending
	call do_signal				
	add esp, 8
.done:								
	mov edx, [esp + 16]
	mov ecx, [esp + 20]
	mov eax, [esp + 24]
	add esp, 28
    iretd

fork_child_trampoline: 						
    pop edx
    pop ecx
    pop eax
    iretd

sys_sigreturn:
	mov eax, [esp + 4]									; struct ucontext *ucontext
	mov ebx, [eax + OFFSET_UCONTEXT_ESP]
	sub ebx, OFFSET_SIGFRAME_ARG						; struct signal_frame *sf

	cmp dword [ebx + OFFSET_SIGFRAME_EIP], 0xC0000000	; check eip
	jae .do_exit

	mov edx, [current]
	mov byte [edx + OFFSET_TASK_CURRENT_SIG], 0			; current_signal clear

	push dword [eax + OFFSET_UCONTEXT_SS]				; ss
	push dword [ebx + OFFSET_SIGFRAME_ESP]				; esp

	mov edx, [eax + OFFSET_UCONTEXT_EFLAGS]				; ucontext->eflags
	mov ecx, [ebx + OFFSET_SIGFRAME_EFLAGS]				; sf->eflags
	and edx, ~EFLAGS_USER_MASK							; ucontext->eflags & ~EFLAGS_USER_MASK
	and ecx, EFLAGS_USER_MASK							; sf->eflags & EFLAGS_USER_MASK
	or edx, ecx											; ucontext->eflags | sf->eflags

	push edx											; eflags
	push dword [eax + OFFSET_UCONTEXT_CS]				; cs
	push dword [ebx + OFFSET_SIGFRAME_EIP]				; eip

	mov eax, [ebx + OFFSET_SIGFRAME_EAX]				; eax
	mov ecx, [ebx + OFFSET_SIGFRAME_ECX]				; ecx
	mov edx, [ebx + OFFSET_SIGFRAME_EDX]				; edx
	mov esi, [ebx + OFFSET_SIGFRAME_ESI]				; esi
	mov edi, [ebx + OFFSET_SIGFRAME_EDI]				; edi
	mov ebp, [ebx + OFFSET_SIGFRAME_EBP]				; ebp
	mov ebx, [ebx + OFFSET_SIGFRAME_EBX]				; edx

	iretd												; return user space
.do_exit:
	push SIGSEGV
	call do_exit