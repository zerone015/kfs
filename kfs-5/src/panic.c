#include "printk.h"
#include "tty.h"
#include "panic.h"
#include "paging.h"
#include <stddef.h>

static inline void hex_dump(struct panic_info *panic_info)
{
    uint32_t *esp;

    printk("EIP: %x\n", panic_info->eip);
    printk("EFLAGS: %x\n\n", panic_info->eflags);
    // printk("EIP is at %p\n", panic_info->eip); 나중에 심볼테이블에서 찾아서 함수의 시작주소 찾아서 함수명으로 바꿔야함 << 하려 했는데 답 없다 잘못된 주소로 점프한 경우 어떤 함수였는지 ebp없이 절대 못찾음 
    printk("eax: %x ebx: %x ecx: %x edx: %x\n", panic_info->eax, panic_info->ebx, panic_info->ecx, panic_info->edx);
    printk("esi: %x edi: %x ebp: %x esp: %x\n", panic_info->esi, panic_info->edi, panic_info->ebp, panic_info->esp);
    printk("Stack: ");
    esp = (uint32_t *)(panic_info->esp - 4);
    for (size_t i = 1; i <= 24; i++) {
        if (esp + i >= (uint32_t *)&stack_top)
            break;
        printk("%x ", esp[i]);
        if (i % 8 == 0 && i != 24 && esp + i + 1 < (uint32_t *)&stack_top)
            printk("\n       ");
    }
    printk("\n\n");
}

static inline void panic_msg_print(const char *msg)
{
    printk("Kernel panic: %s\n", msg);
}

static inline void registers_clear(void)
{
    __asm__ volatile (
        "xor %%eax, %%eax\n"
        "xor %%ebx, %%ebx\n"
        "xor %%ecx, %%ecx\n"
        "xor %%edx, %%edx\n"
        "xor %%esi, %%esi\n"
        "xor %%edi, %%edi\n"
        "xor %%ebp, %%ebp\n"
        "xor %%esp, %%esp\n"
        ::: "memory"
    );
}

static inline void system_halt(void)
{
    __asm__ volatile (
        "cli\n"
        "1:\n"
        "hlt\n"
        "jmp 1b"
        ::: "memory"
    );
}

void __attribute__((noreturn)) panic(const char *msg, struct panic_info *panic_info)
{
    tty_clear();
    hex_dump(panic_info);
    panic_msg_print(msg);
    vga_disable_cursor();
    registers_clear();
    system_halt();
    __builtin_unreachable();
}

void __attribute__((naked, noreturn)) do_panic(__attribute__((unused)) const char *msg)
{
    __asm__ volatile (
        "pushfl\n\t"
        "pushl 4(%esp)\n\t"
        "pushal\n\t"
        "addl $16, 12(%esp)\n\t"
        "pushl %esp\n\t"
        "pushl 48(%esp)\n\t"
        "call panic\n\t"
    );
}