#ifndef _PANIC_H
#define _PANIC_H

#include <stdint.h>

extern char stack_top;

#define K_STACK_TOP (&stack_top)

struct general_purpose_registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
};

struct panic_info {
    struct general_purpose_registers gpr;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

extern void panic(const char *msg, struct panic_info info);
extern void panic_trigger(const char *msg);

#endif