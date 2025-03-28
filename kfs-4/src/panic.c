#include "printk.h"
#include "tty.h"
#include "panic.h"
#include "paging.h"
#include <stddef.h>

static inline void __hex_dump(struct panic_info *info)
{
    uint32_t *esp;

    info->gpr.esp += offsetof(struct panic_info, eflags) - offsetof(struct general_purpose_registers, eax);
    printk("EIP: %x\n", info->eip);
    printk("EFLAGS: %x\n\n", info->eflags);
    // printk("EIP is at %p\n", info->eip); 나중에 심볼테이블에서 찾아서 함수의 시작주소 찾아서 함수명으로 바꿔야함 << 하려 했는데 답 없다 잘못된 주소로 점프한 경우 어떤 함수였는지 ebp없이 절대 못찾음 
    printk("eax: %x ebx: %x ecx: %x edx: %x\n", info->gpr.eax, info->gpr.ebx, info->gpr.ecx, info->gpr.edx);
    printk("esi: %x edi: %x ebp: %x esp: %x\n", info->gpr.esi, info->gpr.edi, info->gpr.ebp, info->gpr.esp);
    printk("Stack: ");
    esp = (uint32_t *)(info->gpr.esp - 4);
    for (size_t i = 1; i <= 24; i++) {
        if (esp + i >= (uint32_t *)K_STACK_TOP)
            break;
        printk("%x ", esp[i]);
        if (i % 8 == 0 && i != 24 && esp + i + 1 < (uint32_t *)K_STACK_TOP)
            printk("\n       ");
    }
    printk("\n\n");
}

void panic(const char *msg, struct panic_info info)
{
    tty_clear_screen();
    __hex_dump(&info);
    printk("Kernel panic: %s\n", msg);
    vga_disable_cursor();
    __asm__ volatile (
        "cli\n"
        "1:\n"
        "hlt\n"
        "jmp 1b"
    );
}