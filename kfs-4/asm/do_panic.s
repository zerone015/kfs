section .text
extern panic
global do_panic
do_panic:
    pushfd
    push cs
    push dword [esp + 8]
    pushad
    add dword [esp + 12], 8
    push esp
    push dword [esp + 52]
    call panic