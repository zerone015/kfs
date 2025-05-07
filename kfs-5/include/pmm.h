#ifndef _PMM_H
#define _PMM_H

#include "multiboot.h"
#include "paging.h"
#include "pmm_types.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_MMAP			50
#define MAX_BLOCK_SIZE		0x00400000U
#define MAX_ORDER			__builtin_ffs(MAX_BLOCK_SIZE / PAGE_SIZE)
#define K_PLOAD_START		((size_t)(&_kernel_start))
#define K_PLOAD_END			((size_t)(&_kernel_end))
#define KERNEL_SIZE			(K_PLOAD_END - K_PLOAD_START)
#define PAGE_NONE			((page_t)-1)

struct buddy_order {
	uint32_t *bitmap;
	size_t free_count;
};

struct buddy_allocator {
	struct buddy_order orders[MAX_ORDER];
};

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