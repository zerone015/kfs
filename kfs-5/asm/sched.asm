%include "offsets.inc"

global switch_to_task
extern current
extern tss
section .text
switch_to_task:
    push ebx
    push esi
    push edi
    push ebp

    mov esi, [current]
    mov [esi + OFFSET_TASK_ESP], esp     

    mov edi, [esp + 20]
    mov [current], edi

    mov esp, [edi + OFFSET_TASK_ESP]
    mov eax, [edi + OFFSET_TASK_CR3]        
    mov edx, [edi + OFFSET_TASK_ESP0]
    mov ebx, [tss]
    mov [ebx + OFFSET_TSS_ESP0], edx
    mov ecx, cr3

    cmp eax, ecx
    je .doneVAS
    mov cr3, eax
    
.doneVAS:
    pop ebp
    pop edi
    pop esi
    pop ebx

    ret