#ifndef _VMM_H
#define _VMM_H

#include "rbtree.h"
#include "pmm.h"
#include <stdint.h>

#define VIRT_KERNEL_START	(0xC0000000 | KERNEL_START)
#define VIRT_KERNEL_END		(0xC0000000 | KERNEL_END)

struct k_virtual_space {
	struct rb_node *by_addr;
	struct rb_node *by_size;
	uint32_t 		addr;
	uint32_t		size;
	uint32_t		left_max;
};

void vmm_init(void);
uint32_t page_map(uint32_t p_addr, size_t size, uint32_t mode);

#endif