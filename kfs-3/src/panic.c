#include "printk.h"
#include "panic.h"

void panic(const char *msg)
{
	printk(KERN_ERR "PANIC: %s\n", msg);
	__asm__("cli; hlt");
}