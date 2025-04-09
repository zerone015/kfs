#ifndef _PANIC_H
#define _PANIC_H

extern char stack_top;

#include <stdint.h>
#include <stdbool.h>

struct panic_info {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, eflags;  
};

__attribute__((noreturn)) void panic(const char *msg, struct panic_info *panic_info);
__attribute__((noreturn)) void do_panic(const char *msg);

#endif