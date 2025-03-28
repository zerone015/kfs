section .data
    error_msg db "!!panic test!!", 0 
section .text
extern panic_trigger
global panic_test
global panic_test2
panic_test1:
    push 15
    push 14
    push 13
    push 12
    push 11
    push 10
    mov eax, 0
    mov ecx, 2
    mov edx, 3
    mov ebx, 1
    mov ebp, 6
    mov esi, 4
    mov edi, 5
    push error_msg
    call panic_trigger
panic_test2:
    push 15
    push 14
    push 13
    push 12
    push 11
    push 10
    mov eax, 0
    mov ecx, 2
    mov edx, 3
    mov ebx, 1
    mov ebp, 6
    mov esi, 4
    mov edi, 5
    jmp 0xDEADBEFF