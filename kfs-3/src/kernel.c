#include "multiboot.h"
#include "vga.h"
#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"
#include "pmm.h"
#include "panic.h"
#include "utils.h"

void kmain(multiboot_info_t* mbd, uint32_t magic, uint16_t *vga_memory)
{
	vga_init();
	tty_init(vga_memory);
	gdt_init();
	idt_init();
	pic_init();
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
		panic("Invalid magic number!");
	if (!CHECK_FLAG(mbd->flags, 6))
		panic("invalid memory map given by GRUB bootloader");
	frame_allocator_init(mbd);
}