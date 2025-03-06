#include "multiboot.h"
#include "vga.h"
#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"
#include "printk.h"
#include "utils.h"
#include "pmm.h"

void frame_allocator_init(multiboot_info_t* mbd)
{
	multiboot_memory_map_t *mmmt;

	for (int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		mmmt = (multiboot_memory_map_t *)((mbd->mmap_addr | 0xC0000000) + i);
		if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
			// 여기서 사용가능한 메모리들을 다 세팅해야한다.
            //		mmmt->addr_low, mmmt->len_low, mmmt->size, mmmt->type);
		}
	}
}

void kmain(multiboot_info_t* mbd, uint32_t magic) 
{
	
	vga_init();
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		printk(KERN_ERR "Invalid magic number: %p\n", magic);
		return;
	}
	if (!CHECK_FLAG(mbd->flags, 6)) {
		printk(KERN_ERR "invalid memory map given by GRUB bootloader\n");
		return;
	}
	frame_allocator_init(mbd);
}
