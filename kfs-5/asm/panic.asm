section .text
global do_panic
extern panic
do_panic:
    pushfd
    push dword [esp + 4]
    pushad
    add dword [esp + 12], 16
    push esp
    push dword [esp + 48]
    call panic