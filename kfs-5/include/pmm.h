#ifndef _PMM_H
#define _PMM_H

#include "multiboot.h"
#include "paging.h"
#include <stdint.h>
#include <stddef.h>

extern char _kernel_start;
extern char _kernel_end;
extern uint64_t ram_size;

#define MAX_MMAP			50
#define MAX_BLOCK_SIZE		0x00400000U
#define MAX_ORDER			__builtin_ffs(MAX_BLOCK_SIZE / PAGE_SIZE)
#define K_PLOAD_START		((size_t)(&_kernel_start))
#define K_PLOAD_END			((size_t)(&_kernel_end))
#define KERNEL_SIZE			(K_PLOAD_END - K_PLOAD_START)
#define PAGE_NONE			((uintptr_t)-1)

#define block_size(order)		(PAGE_SIZE << (order))
#define bitmap_first_size(ram)  (((((ram) + PAGE_SIZE - 1) / PAGE_SIZE) + 7) / 8)

struct buddy_order {
	uint32_t *bitmap;
	size_t free_count;
};

struct buddy_allocator {
	struct buddy_order orders[MAX_ORDER];
};

void pmm_init(multiboot_info_t* mbd);
uintptr_t alloc_pages(size_t size);
void free_pages(uintptr_t addr, size_t size);

#endif