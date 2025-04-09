#ifndef _VMM_H
#define _VMM_H

#include "list.h"
#include "rbtree.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

#define K_VBLOCK_MAX    	((K_VSPACE_SIZE / K_PAGE_SIZE / 2) + 1)
#define K_VBLOCK_MAX_SIZE  	(K_VBLOCK_MAX * sizeof(struct kernel_vblock))
#define VB_ALLOC_FAILED		((void *)-1)

struct kernel_vblock {
	uintptr_t base;
	size_t size;
	struct list_head list_head;
};

struct ptr_stack {
	struct kernel_vblock *ptrs[K_VBLOCK_MAX];
	int top;
};

struct user_vblock {
	struct rb_node by_base;
	struct rb_node by_size;
	uintptr_t base;
	size_t size;
};

struct mapping_file {
	struct rb_node by_base;
	uintptr_t base;
	size_t size;
};

struct user_vblock_tree {
	struct rb_root by_base;
	struct rb_root by_size;
};

struct mapping_file_tree {
	struct rb_root by_base;
};

uintptr_t vmm_init(void);
uintptr_t pages_initmap(uintptr_t p_addr, size_t size, int flags);
size_t vb_size(void *addr);
void *vb_alloc(size_t size);
void vb_free(void *addr);

#endif