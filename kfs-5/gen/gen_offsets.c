#include <stddef.h>
#include "sched_defs.h"
#include "gdt.h"
#include "signal_defs.h"
#include "kernel.h"
#include "errno.h"
#include <stdio.h>

int main(void) {
    printf("%%ifndef _OFFSETS_H\n");
    printf("%%define _OFFSETS_H\n\n");
    printf("%%define OFFSET_TASK_PID %u\n", offsetof(struct task_struct, pid));
    printf("%%define OFFSET_TASK_READY %u\n", offsetof(struct task_struct, ready));
    printf("%%define OFFSET_TASK_CR3 %u\n", offsetof(struct task_struct, cr3));
    printf("%%define OFFSET_TASK_ESP %u\n", offsetof(struct task_struct, esp));
    printf("%%define OFFSET_TASK_ESP0 %u\n", offsetof(struct task_struct, esp0));
    printf("%%define OFFSET_TASK_CURRENT_SIG %u\n", offsetof(struct task_struct, current_signal));
    printf("%%define OFFSET_TASK_SIG_PENDING %u\n", offsetof(struct task_struct, sig_pending));
    printf("%%define OFFSET_GDT_BASE_LOW %u\n", offsetof(struct gdt_entry, base_low));
    printf("%%define OFFSET_GDT_BASE_MID %u\n", offsetof(struct gdt_entry, base_mid));
    printf("%%define OFFSET_GDT_BASE_HIGH %u\n", offsetof(struct gdt_entry, base_high));
    printf("%%define OFFSET_TSS_ESP0 %u\n", offsetof(struct tss_entry, esp0));
    printf("%%define OFFSET_UCONTEXT_EFLAGS %u\n", offsetof(struct ucontext, eflags));
    printf("%%define OFFSET_UCONTEXT_ESP %u\n", offsetof(struct ucontext, esp));
    printf("%%define OFFSET_UCONTEXT_CS %u\n", offsetof(struct ucontext, cs));
    printf("%%define OFFSET_UCONTEXT_SS %u\n", offsetof(struct ucontext, ss));
    printf("%%define OFFSET_SIGFRAME_EIP %u\n", offsetof(struct signal_frame, eip));
    printf("%%define OFFSET_SIGFRAME_EAX %u\n", offsetof(struct signal_frame, eax));
    printf("%%define OFFSET_SIGFRAME_ECX %u\n", offsetof(struct signal_frame, ecx));
    printf("%%define OFFSET_SIGFRAME_EDX %u\n", offsetof(struct signal_frame, edx));
    printf("%%define OFFSET_SIGFRAME_EBX %u\n", offsetof(struct signal_frame, ebx));
    printf("%%define OFFSET_SIGFRAME_ESI %u\n", offsetof(struct signal_frame, esi));
    printf("%%define OFFSET_SIGFRAME_EDI %u\n", offsetof(struct signal_frame, edi));
    printf("%%define OFFSET_SIGFRAME_EBP %u\n", offsetof(struct signal_frame, ebp));
    printf("%%define OFFSET_SIGFRAME_ESP %u\n", offsetof(struct signal_frame, esp));
    printf("%%define OFFSET_SIGFRAME_EFLAGS %u\n", offsetof(struct signal_frame, eflags));
    printf("%%define OFFSET_SIGFRAME_ARG %u\n", offsetof(struct signal_frame, arg));
    printf("%%define EINTR %u\n", EINTR);
    printf("%%define SIGSEGV %u\n", SIGSEGV);
    printf("%%define EFLAGS_USER_MASK %u\n", EFLAGS_USER_MASK);
    printf("\n%%endif\n");
    return 0;
}