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
extern struct page *pgmap;

#define block_size(order)		(PAGE_SIZE << (order))
#define block_pages(order)  	(1UL << (order))

uintptr_t pmm_init(multiboot_info_t *mbd);
uintptr_t alloc_pages(size_t size);
void free_pages(size_t addr, size_t size);

static inline struct page *page_from_pfn(size_t pfn)
{
	return pgmap + pfn;
}

static inline void pgref_inc(size_t pa)
{
	struct page *page;
	size_t pfn;

	pfn = pfn_from_pa(pa);
	page = page_from_pfn(pfn);
	page->ref++;
}

static inline void pgref_dec(size_t pa)
{
	struct page *page;
	size_t pfn;

	pfn = pfn_from_pa(pa);
	page = page_from_pfn(pfn);
	page->ref--;
}

static inline bool page_is_shared(size_t pa)
{
	struct page *page;
	size_t pfn;

	pfn = pfn_from_pa(pa);
	page = page_from_pfn(pfn);
	return page->ref >= 2 ? true : false;
}

#endif