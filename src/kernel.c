#include "multiboot.h"
#include "vga.h"
#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"
#include "pmm.h"
#include "vmm.h"
#include "hmm.h"
#include "panic.h"
#include "pit.h"
#include "sched.h"
#include "daemon.h"
#include "proc.h"
#include "exec.h"

static void init_arch(void)
{
	vga_init();
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
}

static void check_bootloader(multiboot_info_t *mbd, uint32_t magic)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        do_panic("Invalid magic number");
    if (!check_flag(mbd->flags, 6))
        do_panic("Invalid memory map given by GRUB bootloader");
}

static void init_memory(multiboot_info_t *mbd)
{
    pmm_init(mbd);              
    hmm_init(vmm_init());             
    page_ref_init();
}

static void init_scheduler(void)
{
    pit_init();
    tss_init();
    proc_init();
    init_process(); 
}

void kmain(multiboot_info_t* mbd, uint32_t magic)
{
	init_arch();
	check_bootloader(mbd, magic);
	init_memory(mbd);
	init_scheduler();
}