#ifndef _PMM_H
#define _PMM_H

#include <stdint.h>
#include <stddef.h>

extern struct frame_allocator frame_allocator;
extern struct buddy_allocator buddy_allocator;

#define PAGE_SIZE		0x00001000U
#define PAGE_SHIFT		12
#define MAX_BLOCK_SIZE	0x00020000U
#define MAX_ORDER		6

#define BIT_DISTANCE(begin, end) \
    (((end) - (begin)) << 5)

#define BIT_FIRST_SET_OFFSET(begin, end) \
	((BIT_DISTANCE((begin), (end))) + __builtin_clz(*(end)))

#define BIT_SET(bitmap, offset) \
    do { *(bitmap) |= (0x80000000U >> (offset)); } while (0)
	
#define BIT_UNSET(bitmap, offset) \
    do { *(bitmap) &= ~(0x80000000U >> (offset)); } while (0)


struct buddy_order {
	uint32_t *bitmap;
	int free_count;
};

struct buddy_allocator {
	struct buddy_order free_area[MAX_ORDER];
};

struct frame {
	struct frame *next;
};

struct frame_allocator {
	struct frame *free_stack;
	int free_count;
};

static inline void *frame_alloc(void)
{
	struct frame *frame;
       
	frame = frame_allocator.free_stack;
	frame_allocator.free_stack = frame_allocator.free_stack->next;
	frame_allocator.free_count--;
	return (void *)frame;
}

static inline void frame_free(void *addr)
{
	struct frame *frame;
	
	frame = (struct frame *)addr;
	frame->next = frame_allocator.free_stack;
	frame_allocator.free_stack = frame;
	frame_allocator.free_count++;
}

static inline void *frame_block_alloc(size_t size)
{
	int order;
	uint32_t *bitmap;
	int bit_offset;

	if (size > MAX_BLOCK_SIZE)
		return NULL;
	order = 0;
	while (size > (PAGE_SIZE << order))
		order++;
	for (int i = order; i < MAX_ORDER; i++) {
		if (buddy_allocator.free_area[i].free_count > 0) {
			bitmap = buddy_allocator.free_area[i].bitmap;
			while (!(*bitmap)) 
				bitmap++;
			bit_offset = BIT_FIRST_SET_OFFSET(buddy_allocator.free_area[order].bitmap, bitmap);
			BIT_UNSET(bitmap, __builtin_clz(*bitmap));
			buddy_allocator.free_area[order].free_count--;
			while (i > order) {
				i--;
				bit_offset <<= 1;
				bitmap = buddy_allocator.free_area[i].bitmap;
				BIT_SET(&bitmap[bit_offset >> 5], (bit_offset & 31) + 1);
				buddy_allocator.free_area[i].free_count++;
			}
			return (void *)(bit_offset << PAGE_SHIFT << order);
		}
	}
	return NULL;
}

#endif
