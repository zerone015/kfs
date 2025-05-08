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
#define bitmap_first_size(ram)  (((((ram) + PAGE_SIZE - 1) / PAGE_SIZE) + 7) / 8)

void pmm_init(multiboot_info_t* mbd);
void page_ref_init(void);
page_t alloc_pages(size_t size);
void free_pages(page_t addr, size_t size);

static inline void page_ref_inc(page_t page)
{
	page_ref[pfn_from_page(page)]++;
}

static inline void page_ref_dec(page_t page)
{
	page_ref[pfn_from_page(page)]--;
}

static inline void page_set_shared(page_t page)
{
	page_ref[pfn_from_page(page)] = 2;
}

static inline void page_ref_clear(page_t page)
{
	page_ref[pfn_from_page(page)] = 0;
}

static inline bool page_is_shared(page_t page)
{
	return page_ref[pfn_from_page(page)] >= 2 ? true : false;
}

#endif