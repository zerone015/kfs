#include "printk.h"
#include "panic.h"

void panic(const char *msg)
{
    printk(KERN_ERR "PANIC: %s\n", msg);
    __asm__ volatile (
        "cli\n"
        "1:\n"
        "hlt\n"
        "jmp 1b"
    );
}