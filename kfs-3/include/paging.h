#ifndef _PAGING_H
#define _PAGING_H

#define PAGE_SIZE			0x00001000U
#define PAGE_SHIFT			__builtin_ctz(PAGE_SIZE)
#define MAX_RAM_SIZE		0x100000000U
#define K_PAGE_TAB_END     	0xfffff000
#define K_PAGE_TAB_BEGIN    0xfff00000

#endif