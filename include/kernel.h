#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>

// EFLAGS bit definitions
#define EFLAGS_CF     (1 << 0)   // Carry Flag
#define EFLAGS_PF     (1 << 2)   // Parity Flag
#define EFLAGS_AF     (1 << 4)   // Auxiliary Carry Flag
#define EFLAGS_ZF     (1 << 6)   // Zero Flag
#define EFLAGS_SF     (1 << 7)   // Sign Flag
#define EFLAGS_TF     (1 << 8)   // Trap Flag
#define EFLAGS_IF     (1 << 9)   // Interrupt Enable Flag (only modifiable in ring 0)
#define EFLAGS_DF     (1 << 10)  // Direction Flag
#define EFLAGS_OF     (1 << 11)  // Overflow Flag
#define EFLAGS_IOPL   (3 << 12)  // I/O Privilege Level (bits 12 and 13)
#define EFLAGS_NT     (1 << 14)  // Nested Task Flag
#define EFLAGS_RF     (1 << 16)  // Resume Flag
#define EFLAGS_VM     (1 << 17)  // Virtual 8086 Mode
#define EFLAGS_AC     (1 << 18)  // Alignment Check
#define EFLAGS_VIF    (1 << 19)  // Virtual Interrupt Flag
#define EFLAGS_VIP    (1 << 20)  // Virtual Interrupt Pending
#define EFLAGS_ID     (1 << 21)  // ID Flag (CPUID enable)

// User-mode changeable EFLAGS bits
#define EFLAGS_USER_MASK ( \
    EFLAGS_CF  | \
    EFLAGS_PF  | \
    EFLAGS_AF  | \
    EFLAGS_ZF  | \
    EFLAGS_SF  | \
    EFLAGS_TF  | \
    EFLAGS_DF  | \
    EFLAGS_OF    \
)

struct ucontext {
    uint32_t ebp, edi, esi, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
    uint32_t esp, ss;
};

#endif