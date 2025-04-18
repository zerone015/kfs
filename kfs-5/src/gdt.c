#include "gdt.h"
#include "hmm.h"
#include "panic.h"

static struct gdt_entry gdt[GDT_SIZE];
struct tss_entry *tss;

static void gdt_set_entry(size_t idx, uint32_t limit, uint32_t base, uint8_t access, uint8_t flags)
{
	gdt[idx].limit_low = limit & 0xFFFF;
	gdt[idx].base_low = base & 0xFFFF;
	gdt[idx].base_mid = (base >> 16) & 0xFF;
	gdt[idx].access = access;
	gdt[idx].limit_high = (limit >> 16) & 0xF;
	gdt[idx].flags = flags & 0xF;
	gdt[idx].base_high = base >> 24;
}

static inline void gdt_load(struct gdt_ptr *gdt_ptr)
{
	__asm__ (
        "lgdt (%0)\n\t"
        "jmp $" STR(GDT_SELECTOR_CODE_PL0) ", $.reload_cs\n\t"
        ".reload_cs:\n\t"
        "mov $" STR(GDT_SELECTOR_DATA_PL0) ", %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss"
        :
        : "r"(gdt_ptr)
		: "memory"
    );
}

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
	gdt_load(&gdt_ptr);
}

void tss_init(void)
{
	struct tss_entry *tss_entry;

	tss_entry = kmalloc(sizeof(struct tss_entry));
	if (!tss_entry)
		do_panic("tss initialize failed");
	tss_entry->ss0 = GDT_SELECTOR_DATA_PL0;
	tss_entry->esp0 = (uint32_t)&stack_top;
	tss_entry->iomap = sizeof(struct tss_entry);
	gdt_set_entry(5, sizeof(struct tss_entry), (uint32_t)tss_entry, GDT_TSS_ACCESS, GDT_TSS_FLAGS);
	__asm__ ("ltr %%ax" :: "a"(GDT_SELECTOR_TSS));
	tss = tss_entry;
}