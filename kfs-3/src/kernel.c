#include "multiboot.h"
#include "vga.h"
#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"
#include "printk.h"
#include "utils.h"

void kmain(multiboot_info_t* mbd, uint32_t magic) 
{
	int i;
	
	vga_init();
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
	/*
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		printk(KERN_ERR "Invalid magic number: %p\n", magic);
		return;
	}
	if (!CHECK_FLAG(mbd->flags, 6)) {
		printk(KERN_ERR "invalid memory map given by GRUB bootloader\n");
		return;
	}
	gdt_init();
	idt_init();
	pic_init();

	for(i = 0; i < mbd->mmap_length;
        i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt =
            (multiboot_memory_map_t*) (mbd->mmap_addr + i);
	
        printk("Start Addr: %x | Length: %x | Size: %x | Type: %d\n",
            mmmt->addr_low, mmmt->len_low, mmmt->size, mmmt->type);
	printk("Start Addr: %x | Length: %x | Size: %x | Type: %d\n",
	    mmmt->addr_high, mmmt->len_high, mmmt->size, mmmt->type);

        if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
        }
    }
    */
}
