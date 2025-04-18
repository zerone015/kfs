#ifndef _PANIC_H
#define _PANIC_H

extern char stack_top;

#include <stdint.h>
#include <stdbool.h>

struct panic_info {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, eflags;  
};

void __attribute__((noreturn)) panic(const char *msg, struct panic_info *panic_info);
void do_panic(const char *msg);

#endif