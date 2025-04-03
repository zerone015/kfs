ENTRY (_start)

SECTIONS
{
    . = 0x00100000;

	_kernel_start = .;
	.multiboot.data : 
	{
        *(.multiboot.data)
    }

    .multiboot.text : 
	{
	    *(.multiboot.text)
	}

	. += 0xC0000000;

	.text : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text)
	}
	.rodata : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	.data : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}
	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
	_kernel_end = . - 0xC0000000;
}