#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>

#define PAGE_SIZE			(1024U * 4U)
#define K_PAGE_SIZE         (1024U * 1024U * 4U)
#define PAGE_SHIFT			__builtin_ctz(PAGE_SIZE)
#define MAX_RAM_SIZE		0x100000000U
#define K_PAGE_DIR_BEGIN    0xFFFFFC00
#define K_PAGE_DIR_END      0xFFFFFFFF

#define PG_PRESENT          0x001
#define PG_RDWR             0x002
#define PG_PS               0x80
#define PG_GLOBAL           0x100
#define PG_RESERVED         0x800
#define PG_CONTIGUOUS       0x200

#define PG_RESERVED_ENTRY    (PG_RESERVED | PG_PS | PG_RDWR)

#define addr_from_tab(tab)          ((((uint32_t)(tab) & 0x003FF000) << 10) | (((uint32_t)(tab) & 0x00000FFF) << 10))
#define tab_from_addr(addr)         (0xFFC00000 | (((uint32_t)(addr) & 0xFFC00000) >> 10) | (((uint32_t)(addr) & 0x003FF000) >> 10))
#define addr_from_dir(dir)          (((uint32_t)(dir) & 0x00000FFF) << 20)
#define dir_from_addr(addr)         (0xFFFFF000 | (((uint32_t)(addr) & 0xFFC00000) >> 20))
#define addr_erase_offset(addr)     ((uint32_t)(addr) & 0xFFFFF000)
#define addr_get_offset(addr)       ((uint32_t)(addr) & 0x00000FFF)
#define k_addr_erase_offset(addr)   ((uint32_t)(addr) & 0xFFC00000)
#define k_addr_get_offset(addr)     ((uint32_t)(addr) & 0x003FFFFF)

static inline void tlb_flush_all(void) 
{
    uint32_t cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile ("mov %0, %%cr3" :: "r" (cr3));
}

static inline void tlb_flush(uint32_t addr) 
{
    __asm__ volatile ("invlpg (%0)" :: "r" (addr));
}

#endif