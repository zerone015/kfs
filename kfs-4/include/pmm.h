#ifndef _PMM_H
#define _PMM_H

#include "multiboot.h"
#include "paging.h"
#include <stdint.h>
#include <stddef.h>

extern char _kernel_start;
extern char _kernel_end;

#define MAX_MMAP			50
#define MAX_BLOCK_SIZE		0x00400000U
#define MAX_ORDER			__builtin_ffs(MAX_BLOCK_SIZE / PAGE_SIZE)
#define K_PLOAD_START		((size_t)(&_kernel_start))
#define K_PLOAD_END			((size_t)(&_kernel_end))
#define KERNEL_SIZE			(K_PLOAD_END - K_PLOAD_START)

struct buddy_order {
	uint32_t *bitmap;
	size_t free_count;
};

struct buddy_allocator {
	struct buddy_order orders[MAX_ORDER];
};

#define __block_size(order)					(PAGE_SIZE << (order))
#define __bitmap_first_size(ram)    		(((((ram) + PAGE_SIZE - 1) / PAGE_SIZE) + 7) / 8)
#define __bit_distance(begin, end)			(((end) - (begin)) << 5)
#define __bit_first_set_offset(begin, end)	((__bit_distance((begin), (end))) + __builtin_clz(*(end)))
#define __bit_set(bitmap, offset) 			do { *(bitmap) |= (0x80000000U >> (offset)); } while (0)
#define __bit_unset(bitmap, offset) 		do { *(bitmap) &= ~(0x80000000U >> (offset)); } while (0)
#define __bit_check(bitmap, offset) 		((*(bitmap) & (0x80000000U >> (offset))) != 0)

extern void pmm_init(multiboot_info_t* mbd);
extern uint32_t alloc_pages(size_t size);
extern void free_pages(uint32_t addr, size_t size);

#endif