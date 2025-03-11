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
#define MAX_BLOCK_SIZE		0x00020000U
#define MAX_ORDER			__builtin_ffs(MAX_BLOCK_SIZE / PAGE_SIZE)

struct buddy_order {
	uint32_t *bitmap;
	size_t free_count;
};
struct buddy_allocator {
	struct buddy_order orders[MAX_ORDER];
};

#define BIT_DISTANCE(begin, end) \
    (((end) - (begin)) << 5)
#define BIT_FIRST_SET_OFFSET(begin, end) \
	((BIT_DISTANCE((begin), (end))) + __builtin_clz(*(end)))
#define BIT_SET(bitmap, offset) \
    do { *(bitmap) |= (0x80000000U >> (offset)); } while (0)
#define BIT_UNSET(bitmap, offset) \
    do { *(bitmap) &= ~(0x80000000U >> (offset)); } while (0)
#define BIT_CHECK(bitmap, offset) ((*(bitmap) & (0x80000000U >> (offset))) != 0)

void frame_allocator_init(multiboot_info_t* mbd);

static inline __attribute__((always_inline)) void *frame_alloc(size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t bit_offset;

	if (size > MAX_BLOCK_SIZE)
		return NULL;
	order = 0;
	while (size > (PAGE_SIZE << order))
		order++;
	for (size_t i = order; i < MAX_ORDER; i++) {
		if (buddy_allocator.orders[i].free_count) {
			bitmap = buddy_allocator.orders[i].bitmap;
			while (!(*bitmap))
				bitmap++;
			bit_offset = BIT_FIRST_SET_OFFSET(buddy_allocator.orders[i].bitmap, bitmap);
			BIT_UNSET(bitmap, __builtin_clz(*bitmap));
			buddy_allocator.orders[i].free_count--;
			while (i > order) {
				i--;
				bit_offset <<= 1;
				bitmap = buddy_allocator.orders[i].bitmap;
				BIT_SET(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31);
				buddy_allocator.orders[i].free_count++;
			}
			return (void *)(bit_offset << (PAGE_SHIFT + order));
		}
	}
	return NULL;
}

static inline __attribute__((always_inline)) void frame_free(void *addr, size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t bit_offset;

	if (!addr || size > MAX_BLOCK_SIZE)
		return;
	order = 0;
	while (size > (PAGE_SIZE << order))
		order++;
	bit_offset = (uint32_t)addr >> (PAGE_SHIFT + order);
	bitmap = buddy_allocator.orders[order].bitmap;
	while (order < MAX_ORDER - 1) {
		if (!BIT_CHECK(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31))
			break;
		BIT_UNSET(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31);
		buddy_allocator.orders[order].free_count--;
		bit_offset >>= 1;
		order++;
		bitmap = buddy_allocator.orders[order].bitmap;
	}
	BIT_SET(&bitmap[bit_offset >> 5], bit_offset & 31);
	buddy_allocator.orders[order].free_count++;
}

#endif