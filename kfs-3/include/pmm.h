#ifndef _PMM_H
#define _PMM_H

#include "multiboot.h"
#include "paging.h"
#include <stdint.h>
#include <stddef.h>

extern char _kernel_start;
extern char _kernel_end;
extern struct buddy_allocator buddy_allocator;

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

#define block_size(order)					(PAGE_SIZE << (order))
#define bitmap_first_size(ram)    			(((((ram) + PAGE_SIZE - 1) / PAGE_SIZE) + 7) / 8)
#define bit_distance(begin, end)			(((end) - (begin)) << 5)
#define bit_first_set_offset(begin, end)	((bit_distance((begin), (end))) + __builtin_clz(*(end)))
#define bit_set(bitmap, offset) 			do { *(bitmap) |= (0x80000000U >> (offset)); } while (0)
#define bit_unset(bitmap, offset) 			do { *(bitmap) &= ~(0x80000000U >> (offset)); } while (0)
#define bit_check(bitmap, offset) 			((*(bitmap) & (0x80000000U >> (offset))) != 0)

extern void pmm_init(multiboot_info_t* mbd);

static inline uint32_t frame_alloc(size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t bit_offset;

	if (size > MAX_BLOCK_SIZE)
		return 0;
	order = 0;
	while (size > block_size(order))
		order++;
	for (size_t i = order; i < MAX_ORDER; i++) {
		if (buddy_allocator.orders[i].free_count) {
			bitmap = buddy_allocator.orders[i].bitmap;
			while (!*bitmap)
				bitmap++;
			bit_offset = bit_first_set_offset(buddy_allocator.orders[i].bitmap, bitmap);
			bit_unset(bitmap, __builtin_clz(*bitmap));
			buddy_allocator.orders[i].free_count--;
			while (i > order) {
				i--;
				bit_offset <<= 1;
				bitmap = buddy_allocator.orders[i].bitmap;
				bit_set(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31);
				buddy_allocator.orders[i].free_count++;
			}
			return bit_offset << (PAGE_SHIFT + order);
		}
	}
	return 0;
}

static inline void frame_free(uint32_t addr, size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t bit_offset;

	if (!addr || size > MAX_BLOCK_SIZE)
		return;
	order = 0;
	while (size > block_size(order))
		order++;
	bit_offset = addr >> (PAGE_SHIFT + order);
	bitmap = buddy_allocator.orders[order].bitmap;
	while (order < MAX_ORDER - 1) {
		if (!bit_check(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31))
			break;
		bit_unset(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31);
		buddy_allocator.orders[order].free_count--;
		bit_offset >>= 1;
		order++;
		bitmap = buddy_allocator.orders[order].bitmap;
	}
	bit_set(&bitmap[bit_offset >> 5], bit_offset & 31);
	buddy_allocator.orders[order].free_count++;
}

#endif