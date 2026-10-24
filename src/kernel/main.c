#include "multiboot.h"
#include "vga.h"
#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"
#include "pmm.h"
#include "vmm.h"
#include "kmalloc.h"
#include "panic.h"
#include "pit.h"
#include "sched.h"
#include "init.h"
#include "proc.h"
#include "exec.h"
#ifdef KFS_TEST
#include "test.h"
#endif

static void arch_init(void)
{
	vga_init();
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
}

static void bootloader_check(multiboot_info_t *mbd, uint32_t magic)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        do_panic("Invalid magic number");
    if (!check_flag(mbd->flags, 6))
        do_panic("Invalid memory map given by GRUB bootloader");
}

static void memory_init(multiboot_info_t *mbd)
{
    uintptr_t heap_start;

    heap_start = pmm_init(mbd);
    vmm_init();
    hmm_init(heap_start);
}

static void scheduler_init(void)
{
    pit_init();
    tss_init();
}

static void process_init(void)
{
    proc_init();
    init_process(); 
}

void kmain(multiboot_info_t* mbd, uint32_t magic)
{
	arch_init();
	bootloader_check(mbd, magic);
	memory_init(mbd);
#if defined(KFS_TEST_MALLOC)
	test_malloc();
	test_done();
#elif defined(KFS_TEST_PANIC)
	panic_test1();
#elif defined(KFS_TEST_PANIC_EXCEPTION)
	panic_test2();
#endif
	scheduler_init();
    process_init();
}