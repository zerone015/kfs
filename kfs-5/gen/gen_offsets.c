#include <stddef.h>
#include "sched.h"
#include "gdt.h"
#include <stdio.h>

int main(void) {
    printf("%%ifndef _OFFSETS_H\n");
    printf("%%define _OFFSETS_H\n\n");
    printf("%%define OFFSET_TASK_PID %lu\n", offsetof(struct task_struct, pid));
    printf("%%define OFFSET_TASK_READY %lu\n", offsetof(struct task_struct, ready));
    printf("%%define OFFSET_TASK_CR3 %lu\n", offsetof(struct task_struct, cr3));
    printf("%%define OFFSET_TASK_ESP %lu\n", offsetof(struct task_struct, esp));
    printf("%%define OFFSET_TASK_ESP0 %lu\n", offsetof(struct task_struct, esp0));
    printf("%%define OFFSET_GDT_BASE_LOW %lu\n", offsetof(struct gdt_entry, base_low));
    printf("%%define OFFSET_GDT_BASE_MID %lu\n", offsetof(struct gdt_entry, base_mid));
    printf("%%define OFFSET_GDT_BASE_HIGH %lu\n", offsetof(struct gdt_entry, base_high));
    printf("%%define OFFSET_TSS_ESP0 %lu\n", offsetof(struct tss_entry, esp0));
    printf("\n%%endif\n");
    return 0;
}