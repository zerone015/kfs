#ifndef _RBTREE_H
#define _RBTREE_H

#include "utils.h"
#include <stdint.h>

struct rb_node {
	uint32_t		__rb_parent_color; 	/* parent and color*/
	struct rb_node *rb_left;
	struct rb_node *rb_right;
} __attribute__((aligned(sizeof(int32_t))));


struct rb_root {
	struct rb_node *rb_node;
};

#define	RB_RED			0
#define	RB_BLACK		1

#define __rb_color(pc)		((pc) & 1)
#define __rb_is_black(pc)	__rb_color(pc)
#define __rb_is_red(pc)		(!__rb_color(pc))

#define rb_color(rb)		__rb_color((rb)->__rb_parent_color)
#define rb_is_black(rb)		__rb_is_black((rb)->__rb_parent_color)
#define rb_is_red(rb)		__rb_is_red((rb)->__rb_parent_color)

#define __rb_parent(pc)    ((struct rb_node *)(pc & ~3))
#define rb_parent(rb)		__rb_parent((rb)->__rb_parent_color)

#define	rb_entry(ptr, type, member) container_of(ptr, type, member)

extern void rb_insert_color(struct rb_node *node, struct rb_root *root);

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
	rb->__rb_parent_color = rb_color(rb) + (unsigned long)p;
}

static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p, int color)
{
	rb->__rb_parent_color = (unsigned long)p + color;
}

static inline void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link)
{
	node->__rb_parent_color = (unsigned long)parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

static inline void __rb_change_child(struct rb_node *old, struct rb_node *new, struct rb_node *parent, struct rb_root *root)
{
	if (parent) {
		if (parent->rb_left == old)
			parent->rb_left = new;
		else
			parent->rb_right = new;
	} else
		root->rb_node = new;
}

static inline struct rb_node *__rb_erase(struct rb_node *node, struct rb_root *root)
{
	struct rb_node *child = node->rb_right;
	struct rb_node *tmp = node->rb_left;
	struct rb_node *parent, *rebalance;
	unsigned long pc;

	if (!tmp) {
		/*
		 * Case 1: node to erase has no more than 1 child (easy!)
		 *
		 * Note that if there is one child it must be red due to 5)
		 * and node must be black due to 4). We adjust colors locally
		 * so as to bypass __rb_erase_color() later on.
		 */
		pc = node->__rb_parent_color;
		parent = __rb_parent(pc);
		__rb_change_child(node, child, parent, root);
		if (child) {
			child->__rb_parent_color = pc;
			rebalance = NULL;
		} else
			rebalance = __rb_is_black(pc) ? parent : NULL;
	} else if (!child) {
		/* Still case 1, but this time the child is node->rb_left */
		tmp->__rb_parent_color = pc = node->__rb_parent_color;
		parent = __rb_parent(pc);
		__rb_change_child(node, tmp, parent, root);
		rebalance = NULL;
	} else {
		struct rb_node *successor = child, *child2;

		tmp = child->rb_left;
		if (!tmp) {
			/*
			 * Case 2: node's successor is its right child
			 *
			 *    (n)          (s)
			 *    / \          / \
			 *  (x) (s)  ->  (x) (c)
			 *        \
			 *        (c)
			 */
			parent = successor;
			child2 = successor->rb_right;
		} else {
			/*
			 * Case 3: node's successor is leftmost under
			 * node's right child subtree
			 *
			 *    (n)          (s)
			 *    / \          / \
			 *  (x) (y)  ->  (x) (y)
			 *      /            /
			 *    (p)          (p)
			 *    /            /
			 *  (s)          (c)
			 *    \
			 *    (c)
			 */
			do {
				parent = successor;
				successor = tmp;
				tmp = tmp->rb_left;
			} while (tmp);
			child2 = successor->rb_right;
			parent->rb_left = child2;
			successor->rb_right = child;
			rb_set_parent(child, successor);
		}
		tmp = node->rb_left;
		successor->rb_left = tmp;
		rb_set_parent(tmp, successor);

		pc = node->__rb_parent_color;
		tmp = __rb_parent(pc);
		__rb_change_child(node, successor, tmp, root);

		if (child2) {
			rb_set_parent_color(child2, parent, RB_BLACK);
			rebalance = NULL;
		} else {
			rebalance = rb_is_black(successor) ? parent : NULL;
		}
		successor->__rb_parent_color = pc;
	}
	return rebalance;
}


#endif