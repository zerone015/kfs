#ifndef _PMM_DEFS_H
#define _PMM_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "list.h"

#define MAX_MMAP			50
#define MAX_BLOCK_SIZE		0x00400000U
#define MAX_ORDER			__builtin_ffs(MAX_BLOCK_SIZE / PAGE_SIZE)
#define K_PLOAD_START		((size_t)(&_kernel_start))
#define K_PLOAD_END			((size_t)(&_kernel_end))
#define KERNEL_SIZE			(K_PLOAD_END - K_PLOAD_START)
#define PAGE_NONE			((size_t)-1)

/* struct page flags */
#define PAGE_FREE			(1 << 0)

struct page {
	uint32_t flags;
	uint32_t ref_count;
	struct list_head free_list;
};

struct memory_map {
    multiboot_memory_map_t entries[MAX_MMAP + 2];
    size_t count;
};

struct buddy_order {
	struct list_head free_list;
};

struct buddy_allocator {
	struct buddy_order orders[MAX_ORDER];
};

#endif