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
#define PG_US               0x004
#define PG_PS               0x080
#define PG_GLOBAL           0x100
#define PG_RESERVED         0x800
#define PG_COW_RDWR         0x200
#define PG_COW_RDONLY       0x400
#define PG_CONTIGUOUS       0x200 

#define PF_EC_PRESENT               PG_PRESENT
#define PF_EC_USER                  PG_US
#define PF_PDE_FLAGS_MASK           0x17FF
#define PF_PTE_FLAGS_MASK           (PG_GLOBAL | 0xFF)

#define PAGE_DIR            		((uint32_t *)0xFFFFF000)
#define PAGE_TAB            		((uint32_t *)0xFFC00000)

#define kernel_space(addr)			    ((uintptr_t)(addr) >= K_VSPACE_START)
#define user_space(addr)         	    ((uintptr_t)(addr) < K_VSPACE_START)
#define pgtab_from_pdi(pdi)             (current_pgtab() + (pdi*1024))
#define addr_from_pte(p)                ((((uintptr_t)(p) & 0x003FF000) << 10) | (((uintptr_t)(p) & 0x00000FFF) << 10))
#define pte_from_addr(addr)             ((uint32_t *)(0xFFC00000 | (((uintptr_t)(addr) & 0xFFC00000) >> 10) | (((uintptr_t)(addr) & 0x003FF000) >> 10)))
#define addr_from_pde(p)                (((uintptr_t)(p) & 0x00000FFF) << 20)
#define pde_from_addr(addr)             ((uint32_t *)(0xFFFFF000 | (((uintptr_t)(addr) & 0xFFC00000) >> 20)))
#define page_from_pte(pte)              ((pte) & 0xFFFFF000)    
#define page_4mb_from_pde(pde)          ((pde) & 0xFFC00000)     
#define page_4kb_from_pde(pde)          ((pde) & 0xFFFFF000)
#define pfn_from_page(pg)               ((pg) >> 12)
#define pfn_from_pte(pte)               (page_from_pte(pte) >> 12)    
#define pg_entry_flags(entry)           ((entry & 0x00000FFF))
#define pgdir_index(addr)               ((addr) >> 22)
#define pgtab_allocated(addr)           (*pde_from_addr(addr) != 0)
#define addr_is_mapped(addr)            (pgtab_allocated(addr) && *pte_from_addr(addr) != 0)
#define is_cow(pte)                     (((pte) & (PG_COW_RDONLY | PG_COW_RDWR)) != 0)
#define is_rdonly_cow(pte)              (((pte) & PG_COW_RDONLY) != 0)
#define is_rdwr_cow(pte)                (((pte) & PG_COW_RDWR) != 0)
#define addr_erase_offset(addr)         ((uintptr_t)(addr) & 0xFFFFF000)
#define addr_get_offset(addr)           ((uintptr_t)(addr) & 0x00000FFF)
#define k_addr_erase_offset(addr)       ((uintptr_t)(addr) & 0xFFC00000)
#define k_addr_get_offset(addr)         ((uintptr_t)(addr) & 0x003FFFFF)
#define page_is_present(entry)          ((entry) & PG_PRESENT)
#define pf_is_usermode(ec)              ((ec) & PF_EC_USER)
#define entry_reserved(entry)    	    (((entry) & PG_RESERVED))
#define MAKE_PRESENT_PDE(p)             ((p) = (alloc_pages(K_PAGE_SIZE) | (((p) & PF_PDE_FLAGS_MASK) | PG_PRESENT)))
#define MAKE_PRESENT_PTE(p)             ((p) = (alloc_pages(PAGE_SIZE) | (((p) & PF_PTE_FLAGS_MASK) | PG_PRESENT)))

static inline void tlb_invl(uintptr_t addr) 
{
    __asm__ ("invlpg (%0)" :: "r" (addr));
}

static inline uint32_t *current_pgdir(void)
{
    return PAGE_DIR;
}

static inline uint32_t *current_pgtab(void)
{
    return PAGE_TAB;
}

static inline uint32_t current_cr3(void)
{
	uint32_t cr3;

	__asm__ volatile (
		"movl %%cr3, %0"
		: "=r"(cr3)
	);
	return cr3;
}

#endif