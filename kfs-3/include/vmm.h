#ifndef _VMM_H
#define _VMM_H

#include "list.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

#define K_VSPACE_START		0xC0000000
#define K_VSPACE_END		0xFFFFFFFF
#define K_VSPACE_SIZE		0x40000000
#define K_VLOAD_START		(K_VSPACE_START | K_PLOAD_START)
#define K_VLOAD_END			(K_VSPACE_START | K_PLOAD_END)
#define KVS_MAX_NODE		(K_VSPACE_SIZE / K_PAGE_SIZE / 2)
#define KVS_MAX_SIZE		(KVS_MAX_NODE * sizeof(struct k_vspace))

struct k_vspace {
	uint32_t 			addr;
	uint32_t			size;
	struct list_head	list_head;
};

struct free_stack {
	struct k_vspace *free_nodes[KVS_MAX_NODE + 1];
	int top_index;
};

struct k_vspace_manager {
	struct list_head list_head;
	struct free_stack free_stack;
};

extern void vmm_init(void);
extern uint32_t page_map(uint32_t p_addr, size_t size, uint32_t mode);

static inline void stack_init(struct free_stack *stack)
{
	stack->top_index = -1;
}

static inline void stack_push(struct free_stack *stack, struct k_vspace *free_node)
{
	stack->top_index++;
	stack->free_nodes[stack->top_index] = free_node;
}

static inline bool stack_is_empty(struct free_stack *stack)
{
	return stack->top_index == -1 ? true : false;
}

static inline struct k_vspace *stack_pop(struct free_stack *stack)
{
	if (stack_is_empty(stack))
		return NULL;
	return stack->free_nodes[stack->top_index--];
}

#endif