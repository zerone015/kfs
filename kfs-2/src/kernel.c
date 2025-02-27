#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"

static void kinit(void)
{
	tty_init();
	idt_init();
	pic_init();
	gdt_init();
}

void kmain(void) 
{
	kinit();
}
