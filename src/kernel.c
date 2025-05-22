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
#include "ata.h"

static void init_arch(void)
{
	vga_init();
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
    ata_init();
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

static void ata_driver_test(void)
{
    page_t page = alloc_pages(PAGE_SIZE);
    char *buf = (char *)tmp_vmap(page);
    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';
    buf[3] = 'd';
    buf[4] = 'e';
    buf[5] = 'f';
    buf[6] = '\0';

    ata_write(3, 1,(void *)page);

    page_t page2 = alloc_pages(PAGE_SIZE);
    ata_read(3, 1, (void *)page2);

    printk("test result = %s\n", tmp_vmap(page2));
    while (true);
}

static void init_scheduler(void)
{
    pit_init();
    tss_init();
    proc_init();

    __asm__ ("sti" ::: "memory");
    ata_driver_test();

    // init_process(); 
}

void kmain(multiboot_info_t* mbd, uint32_t magic)
{
	init_arch();
	check_bootloader(mbd, magic);
	init_memory(mbd);
	init_scheduler();
}