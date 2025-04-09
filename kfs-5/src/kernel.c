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
#include "proc.h"
#include "exec.h"

void kmain(multiboot_info_t* mbd, uint32_t magic)
{
	uintptr_t mem;

	vga_init();
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
		do_panic("Invalid magic number");
	if (!check_flag(mbd->flags, 6))
		do_panic("invalid memory map given by GRUB bootloader");
	pmm_init(mbd);
	mem = vmm_init();
	hmm_init(mem);
	tss_init();
	pit_init();
	scheduler_init();
	proc_init();
	init_process();
}