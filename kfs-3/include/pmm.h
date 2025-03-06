#ifndef _PMM_H
#define _PMM_H

#define PAGE_SIZE		0x00001000
#define PAGE_SHIFT		12
#define MAX_BLOCK_SIZE	0x00020000
#define MAX_ORDER		6

#include <stdint.h>
#include <stddef.h>

extern struct frame_allocator frame_allocator;
extern struct buddy_allocator buddy_allocator;

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
	size_t block_size;
	int order;
	uint32_t *bitmap;
	int order_count;
	int bit_length;
	int bit_count;
	int bit_idx;

	if (size > MAX_BLOCK_SIZE)
		return NULL;
	order = 0;
	block_size = PAGE_SIZE;
	while (size > block_size) {
		block_size <<= 1;
		order++;
	}
	if (buddy_allocator.free_area[order].free_count > 0) {
		bitmap = buddy_allocator.free_area[order].bitmap;
		while (!(*bitmap & 0xFFFFFFFF)) 
			bitmap++;
		bit_length = ((bitmap - buddy_allocator.free_area[order].bitmap) << 3) + __builtin_clz(*bitmap);
		*bitmap ^= 0x80000000 >> __builtin_clz(*bitmap);
		buddy_allocator.free_area[order].free_count--;
	}
	else {
		order_count = order + 1;
		while (order_count < MAX_ORDER && buddy_allocator.free_area[order_count].free_count == 0)
			order_count++;
		if (order_count == MAX_ORDER)
			return NULL;
		bitmap = buddy_allocator.free_area[order_count].bitmap;
		while (!(*bitmap & 0xFFFFFFFF)) 
			bitmap++;
		bit_length = ((bitmap - buddy_allocator.free_area[order_count].bitmap) << 3) + __builtin_clz(*bitmap);
		*bitmap ^= 0x80000000 >> __builtin_clz(*bitmap);
		buddy_allocator.free_area[order_count].free_count--;
		while (order < order_count) {
			order_count--;
			bitmap = buddy_allocator.free_area[order_count].bitmap;
			bit_length <<= 1;
			bit_count = bit_length >> 5;
			while (bit_count--)
				bitmap++;
			bit_idx = bit_length - ((bitmap - buddy_allocator.free_area[order_count].bitmap) << 3);
			*bitmap ^= 0x80000000 >> (bit_idx + 1);
			buddy_allocator.free_area[order_count].free_count++;
		}
	}
	return (void *)(bit_length << PAGE_SHIFT << order);
}
#endif