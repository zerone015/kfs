#include "sched.h"
#include "hmm.h"
#include "panic.h"

struct task_struct *current;
struct task_struct *pid_table[PID_TABLE_MAX];
struct list_head ready_queue;

static inline void ready_queue_init(void)
{
    init_list_head(&ready_queue);
}

void scheduler_init(void)
{
    ready_queue_init();
}

void __attribute__((naked, noreturn)) yield(void)
{
    __asm__ volatile (
        "movl %eax, current + " STR(OFFSET_TASK_CTX_EAX) "\n\t"
        "movl %ebx, current + " STR(OFFSET_TASK_CTX_EBX) "\n\t"
        "movl %ecx, current + " STR(OFFSET_TASK_CTX_ECX) "\n\t"
        "movl %edx, current + " STR(OFFSET_TASK_CTX_EDX) "\n\t"
        "movl %esi, current + " STR(OFFSET_TASK_CTX_ESI) "\n\t"
        "movl %edi, current + " STR(OFFSET_TASK_CTX_EDI) "\n\t"
        "movl %ebp, current + " STR(OFFSET_TASK_CTX_EBP) "\n\t"

        "leal 4(%esp), %eax\n\t"
        "movl %eax, current + " STR(OFFSET_TASK_CTX_ESP) "\n\t"

        "pushfl\n\t"
        "popl %eax\n\t"
        "movl %eax, current + " STR(OFFSET_TASK_CTX_EFLAGS) "\n\t"

        "movl (%esp), %eax\n\t"
        "movl %eax, current + " STR(OFFSET_TASK_CTX_EIP) "\n\t"


        "movl ready_queue+0, %eax\n\t"     // eax = entry = ready_queue.next
        "movl (%eax), %ecx\n\t"           // ecx = entry->next
        "movl 4(%eax), %edx\n\t"          // edx = entry->prev
        "movl %edx, 4(%ecx)\n\t"          // entry->next->prev = entry->prev
        "movl %ecx, (%edx)\n\t"           // entry->prev->next = entry->next
    
        "subl $" STR(OFFSET_TASK_READY) ", %eax\n\t"
        "movl %eax, current\n\t"           // current = task

        "movzwl gdt + " STR(5 * 8 + OFFSET_GDT_BASE_LOW) ", %ebx\n\t"
        "movzbl gdt + " STR(5 * 8 + OFFSET_GDT_BASE_MID) ", %ecx\n\t"
        "shll $16, %ecx\n\t"
        "orl %ecx, %ebx\n\t"
        "movzbl gdt + " STR(5 * 8 + OFFSET_GDT_BASE_HIGH) ", %ecx\n\t"
        "shll $24, %ecx\n\t"
        "orl %ecx, %ebx\n\t"
        "movl " STR(OFFSET_TASK_ESP0) "(%eax), %ecx\n\t"
        "movl %ecx, " STR(OFFSET_TSS_ESP0) "(%ebx)\n\t"

        "movl " STR(OFFSET_TASK_CR3) "(%eax), %ecx\n\t"
        "movl %ecx, %cr3\n\t"

        "movl " STR(OFFSET_TASK_CTX_EBX) "(%eax), %ebx\n\t"
        "movl " STR(OFFSET_TASK_CTX_EDX) "(%eax), %edx\n\t"
        "movl " STR(OFFSET_TASK_CTX_ESI) "(%eax), %esi\n\t"
        "movl " STR(OFFSET_TASK_CTX_EDI) "(%eax), %edi\n\t"
        "movl " STR(OFFSET_TASK_CTX_EBP) "(%eax), %ebp\n\t"

        "cmpl $" STR(K_VSPACE_START) ", " STR(OFFSET_TASK_CTX_EIP) "(%eax)\n\t"
        "jb 1f\n\t"

        "pushl " STR(OFFSET_TASK_CTX_EFLAGS) "(%eax)\n\t"
        "popf\n\t"
        "movl " STR(OFFSET_TASK_CTX_ESP) "(%eax), %esp\n\t"
        "pushl " STR(OFFSET_TASK_CTX_EIP) "(%eax)\n\t"

        "movl " STR(OFFSET_TASK_CTX_ECX) "(%eax), %ecx\n\t"
        "movl " STR(OFFSET_TASK_CTX_EAX) "(%eax), %eax\n\t"
        
        "ret\n\t"

        "1:\n\t"
        "pushl $" STR(GDT_SELECTOR_DATA_PL3) "\n\t"
        "pushl " STR(OFFSET_TASK_CTX_ESP) "(%eax)\n\t"
        "pushl " STR(OFFSET_TASK_CTX_EFLAGS) "(%eax)\n\t"
        "pushl $" STR(GDT_SELECTOR_CODE_PL3) "\n\t"
        "pushl " STR(OFFSET_TASK_CTX_EIP) "(%eax)\n\t"

        "pushl " STR(OFFSET_TASK_CTX_ECX) "(%eax)\n\t"
        "movl " STR(OFFSET_TASK_CTX_EAX) "(%eax), %eax\n\t"

        "movw $" STR(GDT_SELECTOR_DATA_PL3) ", %cx\n\t"
        "movw %cx, %ds\n\t"
        "movw %cx, %es\n\t"

        "popl %ecx\n\t"

        "iret\n\t"
    );
}