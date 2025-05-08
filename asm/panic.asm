section .text
global do_panic
extern panic
do_panic:
    pushfd                          ; eflags
    push dword [esp + 4]            ; eip
    pushad                  
    add dword [esp + 12], 16        ; esp
    push esp                        ; panic_info
    push dword [esp + 48]           ; panic message
    call panic