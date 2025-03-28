#ifndef _VMM_H
#define _VMM_H

#include "list.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

#define KVS_MAX_NODE		(K_VSPACE_SIZE / K_PAGE_SIZE / 2)
#define KVS_MAX_SIZE		(KVS_MAX_NODE * sizeof(struct k_vspace))

extern uint32_t vmm_init(void);
extern uint32_t pages_initmap(uint32_t p_addr, size_t size, uint32_t flags);
extern void *vs_alloc(size_t size);
extern void vs_free(void *addr);

struct k_vspace {
	uint32_t 			addr;
	uint32_t			size;
	struct list_head	list_head;
};

struct free_stack {
	struct k_vspace *free_nodes[KVS_MAX_NODE + 1];
	int top_index;
};

struct k_vspace_allocator {
	struct list_head list_head;			
	struct free_stack free_stack;			
};

#endif