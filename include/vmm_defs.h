#ifndef _VMM_DEFS_H
#define _VMM_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include "rbtree.h"
#include "list.h"

#define K_VBLOCK_MAX    	(K_VSPACE_SIZE / PAGE_LARGE_SIZE / 2)

/* Kernel allocatable virtual-address block */
struct k_vblock {
    uintptr_t base;
    size_t size;
    struct list_head list_head;
};

/* Kernel vblock object pool */
struct k_vblock_pool {
    struct k_vblock *objs[K_VBLOCK_MAX];
    int top;
};

/* User allocatable virtual-address block */
struct u_vblock {
    struct rb_node by_base;
    struct rb_node by_size;
    uintptr_t base;
    size_t size;
};

/* User vblock red-black trees */
struct u_vblock_tree {
    struct rb_root by_base;
    struct rb_root by_size;
};

/* Mapped-region virtual-address block */
struct mapped_vblock {
    struct rb_node by_base;
    uintptr_t base;
    size_t size;
};

/* Mapped-region tree */
struct mapped_vblock_tree {
    struct rb_root by_base;
};

enum clean_flags {
    CL_TLB_INVL     = 1 << 0,
    CL_RECYCLE      = 1 << 1,
    CL_MAPPING_FREE = 1 << 2,
};

#endif