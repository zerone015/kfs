#ifndef _VMM_H
#define _VMM_H

#include "list.h"
#include "rbtree.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

#define KVB_MAX    		((K_VSPACE_SIZE / K_PAGE_SIZE / 2) + 1)
#define KVB_MAX_SIZE    (KVB_MAX * sizeof(struct k_vblock))

extern uintptr_t vmm_init(void);
extern uintptr_t pages_initmap(uintptr_t p_addr, size_t size, int flags);
extern size_t vb_size(void *addr);
extern void *vb_alloc(size_t size);
extern void vb_free(void *addr);

struct k_vblock {
	uintptr_t base;
	size_t size;
	struct list_head list_head;
};

struct ptr_stack {
	struct k_vblock *ptrs[KVB_MAX];
	int top;
};

struct k_vblock_allocator {
	struct list_head vblocks;
	struct ptr_stack ptr_stack;
};

struct u_vblock {
	struct rb_node *by_base;
	struct rb_node *by_size;
	uintptr_t base;
	size_t size;
};

struct u_mapped_vblock {
	struct rb_node *by_base;
	uintptr_t base;
	size_t size;
};

struct u_vblock_tree {
	struct rb_root by_base;
	struct rb_root by_size;
};

struct u_mapped_vblock_tree {
	struct rb_root by_base;
};

struct u_vblock_allocator {
	struct u_vblock_tree vblocks;
	struct u_mapped_vblock_tree mapped_vblocks;
};

#endif