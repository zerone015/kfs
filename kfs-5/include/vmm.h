#ifndef _VMM_H
#define _VMM_H

#include "list.h"
#include "rbtree.h"
#include "pmm.h"
#include "hmm.h"
#include <stdint.h>
#include <stdbool.h>

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

uintptr_t vmm_init(void);
uintptr_t pages_initmap(uintptr_t p_addr, size_t size, int flags);
size_t vb_size(void *addr);
void *vb_alloc(size_t size);
void vb_free(void *addr);
void vb_unmap(void *addr);

static inline void vblock_bybase_add(struct user_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->base < rb_entry(parent, struct user_vblock, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_base, parent, cur);
    rb_insert_color(&new->by_base, root);
}

static inline void vblock_bysize_add(struct user_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->size < rb_entry(parent, struct user_vblock, by_size)->size)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_size, parent, cur);
    rb_insert_color(&new->by_size, root);
}

static inline void mapping_file_add(struct mapping_file *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->base < rb_entry(parent, struct mapping_file, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_base, parent, cur);
    rb_insert_color(&new->by_base, root);
}

static inline void vblocks_clear(struct user_vblock_tree *vblocks)
{
    struct rb_root *root;
    struct user_vblock *cur, *tmp;

    root = &vblocks->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        kfree(cur);
    }
    root->rb_node = NULL;
    
    root = &vblocks->by_size;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_size) {
        kfree(cur);
    }
    root->rb_node = NULL;
}

#endif