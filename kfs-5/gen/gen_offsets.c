#include <stddef.h>
#include "sched_types.h"
#include "gdt_types.h"
#include <stdio.h>

int main(void) {
    printf("#ifndef _OFFSETS_H\n");
    printf("#define _OFFSETS_H\n\n");
    printf("#define OFFSET_TASK_READY %lu\n", offsetof(struct task_struct, ready));
    printf("#define OFFSET_TASK_CR3 %lu\n", offsetof(struct task_struct, cr3));
    printf("#define OFFSET_TASK_ESP0 %lu\n", offsetof(struct task_struct, esp0));
    printf("#define OFFSET_TASK_CTX %lu\n", offsetof(struct task_struct, cpu_context));
    printf("#define OFFSET_TASK_CTX_EAX %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, eax));
    printf("#define OFFSET_TASK_CTX_EBX %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, ebx));
    printf("#define OFFSET_TASK_CTX_ECX %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, ecx));
    printf("#define OFFSET_TASK_CTX_EDX %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, edx));
    printf("#define OFFSET_TASK_CTX_ESI %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, esi));
    printf("#define OFFSET_TASK_CTX_EDI %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, edi));
    printf("#define OFFSET_TASK_CTX_EBP %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, ebp));
    printf("#define OFFSET_TASK_CTX_ESP %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, esp));
    printf("#define OFFSET_TASK_CTX_EFLAGS %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, eflags));
    printf("#define OFFSET_TASK_CTX_EIP %lu\n", offsetof(struct task_struct, cpu_context) + offsetof(struct cpu_context, eip));
    printf("#define OFFSET_GDT_BASE_LOW %lu\n", offsetof(struct gdt_entry, base_low));
    printf("#define OFFSET_GDT_BASE_MID %lu\n", offsetof(struct gdt_entry, base_mid));
    printf("#define OFFSET_GDT_BASE_HIGH %lu\n", offsetof(struct gdt_entry, base_high));
    printf("#define OFFSET_TSS_ESP0 %lu\n", offsetof(struct tss, esp0));
    printf("\n#endif\n");
    return 0;
}