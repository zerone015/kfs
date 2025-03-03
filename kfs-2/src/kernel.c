#include "tty.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"

static void kinit(void)
{
	tty_init();
	gdt_init();
	idt_init();
	pic_init();
}

void kmain(void) 
{
	kinit();
}
