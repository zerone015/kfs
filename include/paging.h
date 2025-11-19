#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>

#define PAGE_DIR            0xFFFFF000
#define PAGE_TAB            0xFFC00000

#define PAGE_SIZE			(1024U * 4U)
#define PAGE_LARGE_SIZE     (PAGE_SIZE * 1024U)
#define PAGE_SHIFT			__builtin_ctz(PAGE_SIZE)
#define PAGE_LARGE_SHIFT    __builtin_ctz(PAGE_LARGE_SIZE)
#define MAX_RAM_SIZE		0x100000000U

#define KERNEL_PDE_INDEX  	768
#define KERNEL_PDE_BASE   	(PAGE_DIR + KERNEL_PDE_INDEX * sizeof(uint32_t))

#define K_VSPACE_START		0xC0000000
#define K_VSPACE_SIZE		0x40000000

/* Hardware PTE flags */
#define PG_PRESENT      0x001   /* present */
#define PG_RDWR         0x002   /* writable */
#define PG_US           0x004   /* user accessible */
#define PG_PS           0x080   /* 4MB page */
#define PG_GLOBAL       0x100   /* global */

/* KFS custom flags (use reserved bits) */
#define PG_RESERVED     0x800   /* lazy mapping / reserved entry */
#define PG_COW_RDWR     0x200   /* originally writable */
#define PG_COW_RDONLY   0x400   /* originally read-only */
#define PG_CONTIGUOUS   0x200   /* part of the same virtual block */

#define PTE_FLAGS_MASK 				(PG_GLOBAL | 0xFF)

#define PF_EC_PRESENT               PG_PRESENT
#define PF_EC_USER                  PG_US
#define PF_PDE_FLAGS_MASK           0x17FF
#define PF_PTE_FLAGS_MASK           PTE_FLAGS_MASK

#define user_space(addr)         	    ((uintptr_t)(addr) < K_VSPACE_START)
#define pgtab_from_pdi(pdi)             (pgtab_base() + (pdi*1024))
#define va_from_pte(p)                  ((((uintptr_t)(p) & 0x003FF000) << 10) | (((uintptr_t)(p) & 0x00000FFF) << 10))
#define pte_from_va(addr)               ((uint32_t *)(0xFFC00000 | (((uintptr_t)(addr) & 0xFFC00000) >> 10) | (((uintptr_t)(addr) & 0x003FF000) >> 10)))
#define va_from_pde(p)            		(((uintptr_t)(p) & 0x00000FFF) << 20)
#define pde_from_va(addr)               ((uint32_t *)(0xFFFFF000 | (((uintptr_t)(addr) & 0xFFC00000) >> 20)))
#define pa_from_pte(pte)         		((pte) & 0xFFFFF000)
#define pa_from_pde(pde, size)   		((pde) & ~((size) - 1))
#define pfn_from_pa(p_addr)      		((p_addr) >> PAGE_SHIFT)
#define pa_from_pfn(pfn)      			((pfn) << PAGE_SHIFT)
#define pfn_from_pte(pte)               (pa_from_pte(pte) >> PAGE_SHIFT)    
#define pg_entry_flags(entry)           ((entry & 0x00000FFF))
#define pgdir_index(addr)               ((addr) >> 22)
#define pgtab_allocated(addr)           (*pde_from_va(addr) != 0)
#define addr_is_mapped(addr)            (pgtab_allocated(addr) && *pte_from_va(addr) != 0)
#define is_cow(pte)                     (((pte) & (PG_COW_RDONLY | PG_COW_RDWR)) != 0)
#define is_rdwr_cow(pte)                (((pte) & PG_COW_RDWR) != 0)
#define page_is_present(entry)          ((entry) & PG_PRESENT)
#define pf_is_usermode(ec)              ((ec) & PF_EC_USER)
#define entry_reserved(entry)    	    (((entry) & PG_RESERVED))
#define MAKE_PRESENT_PDE(p)             ((p) = (alloc_pages(PAGE_LARGE_SIZE) | (((p) & PF_PDE_FLAGS_MASK) | PG_PRESENT)))
#define MAKE_PRESENT_PTE(p)             ((p) = (alloc_pages(PAGE_SIZE) | (((p) & PF_PTE_FLAGS_MASK) | PG_PRESENT)))

static inline void tlb_invl(uintptr_t addr) 
{
    __asm__ ("invlpg (%0)" :: "r" (addr));
}

static inline uint32_t *pgdir_base(void)
{
    return (uint32_t *)PAGE_DIR;
}

static inline uint32_t *pgtab_base(void)
{
    return (uint32_t *)PAGE_TAB;
}

static inline uint32_t *pgdir_kernel_base(void)
{
	return (uint32_t *)KERNEL_PDE_BASE;
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