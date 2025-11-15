#ifndef _PMM_H
#define _PMM_H

#include "multiboot.h"
#include "paging.h"
#include "pmm_defs.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern char _kernel_start;
extern char _kernel_end;
extern uint64_t ram_size;
extern uint16_t *page_ref;

#define block_size(order)		(PAGE_SIZE << (order))
#define block_pages(order)  	(1UL << (order))

void pmm_init(multiboot_info_t* mbd);
void page_ref_init(void);
uintptr_t alloc_pages(size_t size);
void free_pages(size_t addr, size_t size);

static inline void page_ref_inc(uintptr_t page)
{
	page_ref[pfn_from_phys_addr(page)]++;
}

static inline void page_ref_dec(uintptr_t page)
{
	page_ref[pfn_from_phys_addr(page)]--;
}

static inline void page_set_shared(uintptr_t page)
{
	page_ref[pfn_from_phys_addr(page)] = 2;
}

static inline void page_ref_clear(uintptr_t page)
{
	page_ref[pfn_from_phys_addr(page)] = 0;
}

static inline bool page_is_shared(uintptr_t page)
{
	return page_ref[pfn_from_phys_addr(page)] >= 2 ? true : false;
}

#endif