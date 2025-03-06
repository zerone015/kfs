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
	void *frame_block;

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
		frame_block = (bitmap - buddy_allocator.free_area[order].bitmap + __builtin_clz(*bitmap)) << PAGE_SHIFT; // 이렇게 하면 안된다 각 오더마다 비트당 크기가 다르다 수정해야함
		*bitmap ^= 0x80000000 >> __builtin_clz(*bitmap);
	}
	else {
		order++;
		while (order < MAX_ORDER && buddy_allocator.free_area[order].free_count == 0)
			order++;
		if (order == MAX_ORDER)
			return NULL;
		// 이제 분할하고 주소를 찾아야한다..
	}
	return frame_block;
}
//페이지갯수 = 비트갯수 = 1kb개
#endif