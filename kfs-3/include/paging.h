#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>

#define PAGE_SIZE			0x00001000U
#define PAGE_SHIFT			__builtin_ctz(PAGE_SIZE)
#define MAX_RAM_SIZE		0x100000000U
#define K_PAGE_TAB_END     	0xfffff000
#define K_PAGE_TAB_BEGIN    0xfff00000

#define PAGE_TAB_RDWR       0x003
#define PAGE_TAB_GLOBAL     0x100

#define addr_from_tab(tab)          ((((uint32_t)(tab) & 0x003FF000) << 10) | ((((uint32_t)(tab) & 0x00000FFF) / 4) << 12))
#define tab_from_addr(addr)         (0xFFC00000 | (((uint32_t)(addr) & 0xFFC00000) >> 10) | ((((uint32_t)(addr) & 0x003FF000) >> 12) * 4))
#define addr_erase_offset(addr)     ((uint32_t)(addr) & 0xFFFFF000)
#define addr_get_offset(addr)       ((uint32_t)(addr) & 0x00000FFF)

static inline void reload_cr3(void) 
{
    uint32_t cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile ("mov %0, %%cr3" :: "r" (cr3));
}

#endif