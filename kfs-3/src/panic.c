#include "printk.h"
#include "tty.h"
#include "panic.h"
#include "paging.h"
#include <stddef.h>

static inline void __hex_dump(struct interrupt_frame *iframe)
{
    uint32_t *esp;

    iframe->gpr.esp += offsetof(struct interrupt_frame, eflags) - offsetof(struct general_purpose_registers, eax);
    printk("EIP: %x\n", iframe->eip);
    printk("EFLAGS: %x\n\n", iframe->eflags);
    // printk("EIP is at %p\n", iframe->eip); 나중에 심볼테이블에서 찾아서 함수의 시작주소 찾아서 함수명으로 바꿔야함 << 하려 했는데 답 없다 잘못된 주소로 점프한 경우 어떤 함수였는지 ebp없이 절대 못찾음 
    printk("eax: %x ebx: %x ecx: %x edx: %x\n", iframe->gpr.eax, iframe->gpr.ebx, iframe->gpr.ecx, iframe->gpr.edx);
    printk("esi: %x edi: %x ebp: %x esp: %x\n", iframe->gpr.esi, iframe->gpr.edi, iframe->gpr.ebp, iframe->gpr.esp);
    printk("Stack: ");
    esp = (uint32_t *)(iframe->gpr.esp - 4);
    for (size_t i = 1; i <= 24; i++) {
        if (esp + i >= (uint32_t *)&stack_top)
            break;
        printk("%x ", esp[i]);
        if (i % 8 == 0 && i != 24 && esp + i + 1 < (uint32_t *)&stack_top)
            printk("\n       ");
    }
    printk("\n\n");
}

static inline void __panic_msg_print(const char *msg)
{
    printk("Kernel panic: %s\n", msg);
}

static inline void __registers_clear(void)
{
    __asm__ volatile (
        "xor %eax, %eax\n"
        "xor %ebx, %ebx\n"
        "xor %ecx, %ecx\n"
        "xor %edx, %edx\n"
        "xor %esi, %esi\n"
        "xor %edi, %edi\n"
        "xor %ebp, %ebp\n"
        "xor %esp, %esp\n"
    );
}

static inline void __system_halt(void)
{
    __asm__ volatile (
        "cli\n"
        "1:\n"
        "hlt\n"
        "jmp 1b"
    );
}

void panic(const char *msg, struct interrupt_frame *iframe)
{
    tty_clear();
    __hex_dump(iframe);
    __panic_msg_print(msg);
    vga_disable_cursor();
    __registers_clear();
    __system_halt();
}