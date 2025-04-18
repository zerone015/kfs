section .text
extern syscall_dispatch
global syscall_handler	
syscall_handler:
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    call syscall_dispatch
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov edx, [esp + 12]
	mov ecx, [esp + 8]
	add esp, 28
    iretd


%include "offsets.inc"

section .text
global stack_copy_and_adjust
extern current
extern memcpy32
stack_copy_and_adjust:
    push ebx
    push esi
    push edi
    push ebp

    mov eax, [current]
    mov ebx, [esp + 20]

    mov edi, [ebx + OFFSET_TASK_ESP0]
    sub edi, 8192
    mov esi, [eax + OFFSET_TASK_ESP0]
    sub esi, 8192
    
    push 2048
    push esi
    push edi
    call memcpy32
    add esp, 12

    mov eax, esp
    sub eax, esi
    add edi, eax
    mov [ebx + OFFSET_TASK_ESP], edi  

    pop ebp
    pop edi
    pop esi
    pop ebx
      
    ret