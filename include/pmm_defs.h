#ifndef _PMM_DEFS_H
#define _PMM_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "list.h"

#define MAX_MMAP			62
#define MAX_BLOCK_SIZE		0x00400000U
#define MAX_ORDER			__builtin_ffs(MAX_BLOCK_SIZE / PAGE_SIZE)
#define K_PLOAD_START		((size_t)(&_kernel_start))
#define K_PLOAD_END			((size_t)(&_kernel_end))
#define KERNEL_SIZE			(K_PLOAD_END - K_PLOAD_START)
#define PAGE_NONE			((size_t)-1)

/* struct page flags */
#define PAGE_FREE			(1 << 0)

struct mmap_buffer {
    multiboot_memory_map_t regions[MAX_MMAP + 2];
    size_t count;
};

struct meminfo {
	size_t usable_pages;
};

struct page {
	uint32_t order;
	uint32_t ref;
	uint32_t flags;
	struct list_head free_list;
};

struct buddy_order {
	struct list_head free_list;
};

struct buddy_allocator {
	struct buddy_order orders[MAX_ORDER];
};

#endif