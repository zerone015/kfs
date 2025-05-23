#ifndef _VMM_DEFS_H
#define _VMM_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include "rbtree.h"
#include "list.h"

#define K_VBLOCK_MAX    	((K_VSPACE_SIZE / K_PAGE_SIZE / 2) + 1)
#define K_VBLOCK_MAX_SIZE  	(K_VBLOCK_MAX * sizeof(struct kernel_vblock))

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

enum clean_flags {
    CL_TLB_INVL     = 1 << 0,
    CL_RECYCLE      = 1 << 1,
    CL_MAPPING_FREE = 1 << 2,
};

#endif