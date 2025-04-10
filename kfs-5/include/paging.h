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

#define U_VSPACE_START      PAGE_SIZE
#define U_VSPACE_SIZE       0xC0000000
#define K_VSPACE_START		0xC0000000
#define K_VSPACE_END		0xFFFFFFFF
#define K_VSPACE_SIZE		0x40000000
#define K_VLOAD_START		(K_VSPACE_START | K_PLOAD_START)
#define K_VLOAD_END			(K_VSPACE_START | K_PLOAD_END)

#define PG_PRESENT          0x001
#define PG_RDWR             0x002
#define PG_US               0x004
#define PG_PS               0x080
#define PG_GLOBAL           0x100
#define PG_RESERVED         0x800
#define PG_COW_RDWR         0x200
#define PG_COW_RDONLY       0x400
#define PG_CONTIGUOUS       0x200  // only kernel space

#define PAGE_DIR            0xFFFFF000
#define PAGE_TAB            0xFFC00000

#define addr_from_pte(p)            ((((uintptr_t)(p) & 0x003FF000) << 10) | (((uintptr_t)(p) & 0x00000FFF) << 10))
#define pte_from_addr(addr)         (0xFFC00000 | (((uintptr_t)(addr) & 0xFFC00000) >> 10) | (((uintptr_t)(addr) & 0x003FF000) >> 10))
#define addr_from_pde(p)            (((uintptr_t)(p) & 0x00000FFF) << 20)
#define pde_from_addr(addr)         (0xFFFFF000 | (((uintptr_t)(addr) & 0xFFC00000) >> 20))
#define page_from_pte(pte)          ((pte) & 0xFFFFF000)    
#define page_from_pde_4mb(pte)      ((pte) & 0xFFC00000)     
#define page_from_pde_4kb(pte)      ((pte) & 0xFFFFF000)
#define pfn_from_pte(pte)           (page_from_pte(pte) >> 12)    
#define flags_from_entry(entry)     ((entry & 0x00000FFF))
#define pde_idx(addr)               ((addr) >> 22)
#define pte_idx(addr)               (((addr) >> 12) & 0x3FF)
#define has_pgtab(addr)             (*((uint32_t *)pde_from_addr((addr) & 0xFFC00000)) != 0)
#define is_rdonly_cow(pte)          (((pte) & PG_COW_RDONLY) != 0)
#define is_rdwr_cow(pte)            (((pte) & PG_COW_RDWR) != 0)
#define addr_erase_offset(addr)     ((uintptr_t)(addr) & 0xFFFFF000)
#define addr_get_offset(addr)       ((uintptr_t)(addr) & 0x00000FFF)
#define k_addr_erase_offset(addr)   ((uintptr_t)(addr) & 0xFFC00000)
#define k_addr_get_offset(addr)     ((uintptr_t)(addr) & 0x003FFFFF)
#define is_kernel_space(addr)       ((uintptr_t)(addr) >= K_VSPACE_START)
#define page_is_present(entry)      ((entry) & PG_PRESENT)

static inline void tlb_flush_all(void) 
{
    uint32_t cr3;

    asm volatile ("mov %%cr3, %0" : "=r" (cr3) :: "memory");
    asm volatile ("mov %0, %%cr3" :: "r" (cr3) : "memory");
}

static inline void tlb_flush(uintptr_t addr) 
{
    asm volatile ("invlpg (%0)" :: "r" (addr) : "memory");
}

#endif