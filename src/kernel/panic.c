#include "printk.h"
#include "tty.h"
#include "panic.h"
#include "paging.h"
#include "sched.h"
#include <stddef.h>

static void hex_dump(struct panic_info *panic_info)
{
    uint32_t *esp;
    uint32_t *esp_top;
    size_t i;

    esp = (uint32_t *)panic_info->esp;
    esp_top = current ? (uint32_t *)current->esp0 : (uint32_t *)&stack_top;

    printk("EIP: %x\n", panic_info->eip);
    printk("EFLAGS: %x\n\n", panic_info->eflags);
    printk("eax: %x ebx: %x ecx: %x edx: %x\n", panic_info->eax, panic_info->ebx, panic_info->ecx, panic_info->edx);
    printk("esi: %x edi: %x ebp: %x esp: %x\n", panic_info->esi, panic_info->edi, panic_info->ebp, panic_info->esp);
   
    printk("Stack: ");
    for (i = 0; i < STACK_DUMP_ENTRIES; i++) {
        if ((esp + i) >= esp_top)
            break;
        printk("%x ", esp[i]);
        if ((i + 1) % ENTRIES_PER_LINE == 0)
            printk("\n       ");
    }
    printk("\n");
    if (((i + 1) % ENTRIES_PER_LINE) || ((esp + i) >= esp_top))
        printk("\n");
}

static void panic_msg_print(const char *msg)
{
    printk("Kernel panic: %s\n", msg);
}

static void system_halt(void)
{
    __asm__ (
        "xor %%eax, %%eax\n\t"
        "xor %%ebx, %%ebx\n\t"
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "xor %%esi, %%esi\n\t"
        "xor %%edi, %%edi\n\t"
        "xor %%ebp, %%ebp\n\t"
        "xor %%esp, %%esp\n\t"
        "cli\n\t"
        "1:\n\t"
        "hlt\n\t"
        "jmp 1b"
        ::: "memory"
    );
}

void panic(const char *msg, struct panic_info *panic_info)
{
    tty_clear();
    hex_dump(panic_info);
    panic_msg_print(msg);
    vga_disable_cursor();
    system_halt();
    __builtin_unreachable();
}