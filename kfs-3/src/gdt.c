#include "gdt.h"

struct gdt_entry gdt[GDT_SIZE];

void gdt_init(void)
{
	struct gdt_ptr gdt_ptr;

	gdt_set_entry(0, 0, 0, 0, 0);
	gdt_set_entry(1, 0xFFFFF, 0, GDT_CODE_PL0, GDT_FLAGS);
	gdt_set_entry(2, 0xFFFFF, 0, GDT_DATA_PL0, GDT_FLAGS);
	gdt_set_entry(3, 0xFFFFF, 0, GDT_CODE_PL3, GDT_FLAGS);
	gdt_set_entry(4, 0xFFFFF, 0, GDT_DATA_PL3, GDT_FLAGS);
	gdt_ptr.limit = sizeof(struct gdt_entry) * GDT_SIZE - 1;
	gdt_ptr.base = (uint32_t)&gdt;
	gdt_load((uint32_t)&gdt_ptr);
}

void gdt_set_entry(size_t idx, uint32_t limit, uint32_t base, uint8_t access, uint8_t flags)
{
	gdt[idx].limit_low = limit & 0xFFFF;
	gdt[idx].base_low = base & 0xFFFF;
	gdt[idx].base_mid = (base >> 16) & 0xFF;
	gdt[idx].access = access;
	gdt[idx].limit_high = (limit >> 16) & 0xF;
	gdt[idx].flags = flags & 0xF;
	gdt[idx].base_high = base >> 24;
}
