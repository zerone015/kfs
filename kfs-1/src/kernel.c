#include "tty.h"
#include "pic.h"
#include "idt.h"

void kernel_main(void) 
{
	tty_init();
	idt_init();
	pic_init();
}
