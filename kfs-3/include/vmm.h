#ifndef _VMM_H
#define _VMM_H

#include "rbtree.h"
#include "pmm.h"
#include <stdint.h>

#define K_VSPACE_START		(0xC0000000)
#define K_VSPACE_END		(0xFFFFFFFF)
#define K_VLOAD_START		(K_VSPACE_START | K_PLOAD_START)
#define K_VLOAD_END			(K_VSPACE_START | K_PLOAD_END)

struct k_vspace {
	struct rb_node	by_addr;
	struct rb_node	by_size;
	uint32_t 		addr;
	uint32_t		size;
};

struct k_vspace_manager {
	struct rb_root rb_aroot;
	struct rb_root rb_sroot;
	struct k_vspace *kvs;
};

extern void vmm_init(void);
extern uint32_t page_map(uint32_t p_addr, size_t size, uint32_t mode);

#endif