#ifndef _VMM_H
#define _VMM_H

#include "list.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

#define KVB_MAX_NODE		(K_VSPACE_SIZE / K_PAGE_SIZE / 2)
#define KVB_MAX_SIZE		(KVB_MAX_NODE * sizeof(struct k_vblock))

extern uintptr_t vmm_init(void);
extern uintptr_t pages_initmap(uintptr_t p_addr, size_t size, int flags);
extern size_t vb_size(void *addr);
extern void *vb_alloc(size_t size);
extern void vb_free(void *addr);

struct k_vblock {
	uintptr_t 			addr;
	size_t				size;
	struct list_head	list_head;
};

struct free_stack {
	struct k_vblock *free_nodes[KVB_MAX_NODE + 1];
	int top_index;
};

struct k_vblock_allocator {
	struct list_head list_head;			
	struct free_stack free_stack;			
};

#endif