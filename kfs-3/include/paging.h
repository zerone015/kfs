#ifndef _PAGING_H
#define _PAGING_H

#define PAGE_SIZE			0x00001000U
#define PAGE_SHIFT			__builtin_ctz(PAGE_SIZE)
#define MAX_RAM_SIZE		0x100000000U
#define K_PAGE_TAB_END     	0xfffff000
#define K_PAGE_TAB_BEGIN    0xfff00000

#define PAGE_TAB_RDWR       0x003

#define ADDR_FROM_TAB(tab)          ((((uint32_t)(tab) & 0x003FF000) << 10) | ((((uint32_t)(tab) & 0x00000FFF) / 4) << 12))
#define TAB_FROM_ADDR(addr)         (0xFFC00000 | (((addr) & 0xFFC00000) >> 10) | ((((addr) & 0x003FF000) >> 12) * 4))
#define ADDR_REMOVE_OFFSET(addr)    ((addr) & 0xFFFFF000)
#define ADDR_GET_OFFSET(addr)       ((addr) & 0x00000FFF)

#endif