#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>

#define PAGE_SIZE			(1024U * 4U)
#define K_PAGE_SIZE         (1024U * 1024U * 4U)
#define PAGE_SHIFT			__builtin_ctz(PAGE_SIZE)
#define K_PAGE_SHIFT        __builtin_ctz(K_PAGE_SIZE)
#define MAX_RAM_SIZE		0x100000000U
#define K_PDE_START         0xFFFFFC00
#define K_PDE_END           0xFFFFFFFF

#define K_VSPACE_START		0xC0000000
#define K_VSPACE_END		0xFFFFFFFF
#define K_VSPACE_SIZE		0x40000000
#define K_VLOAD_START		(K_VSPACE_START | K_PLOAD_START)
#define K_VLOAD_END			(K_VSPACE_START | K_PLOAD_END)

#define PG_PRESENT          0x001
#define PG_RDWR             0x002
#define PG_PS               0x80
#define PG_GLOBAL           0x100
#define PG_RESERVED         0x800
#define PG_CONTIGUOUS       0x200

#define PG_RESERVED_ENTRY    (PG_RESERVED | PG_PS | PG_RDWR)

#define addr_from_pte(pte)          ((((uintptr_t)(pte) & 0x003FF000) << 10) | (((uintptr_t)(pte) & 0x00000FFF) << 10))
#define pte_from_addr(addr)         (0xFFC00000 | (((uintptr_t)(addr) & 0xFFC00000) >> 10) | (((uintptr_t)(addr) & 0x003FF000) >> 10))
#define addr_from_pde(dir)          (((uintptr_t)(dir) & 0x00000FFF) << 20)
#define pde_from_addr(addr)         (0xFFFFF000 | (((uintptr_t)(addr) & 0xFFC00000) >> 20))
#define addr_erase_offset(addr)     ((uintptr_t)(addr) & 0xFFFFF000)
#define addr_get_offset(addr)       ((uintptr_t)(addr) & 0x00000FFF)
#define k_addr_erase_offset(addr)   ((uintptr_t)(addr) & 0xFFC00000)
#define k_addr_get_offset(addr)     ((uintptr_t)(addr) & 0x003FFFFF)
#define is_kernel_space(addr)       ((uintptr_t)(addr) >= K_VSPACE_START)

static inline void tlb_flush_all(void) 
{
    uint32_t cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile ("mov %0, %%cr3" :: "r" (cr3));
}

static inline void tlb_flush(uintptr_t addr) 
{
    __asm__ volatile ("invlpg (%0)" :: "r" (addr));
}

#endif