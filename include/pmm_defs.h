#ifndef _PMM_DEFS_H
#define _PMM_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"

typedef uintptr_t page_t;

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

#endif