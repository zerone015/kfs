#include "page_fault_handle.h"
#include "pmm.h"
#include "paging.h"

void page_fault_handle(uint32_t error_code) 
{
    uint32_t fault_addr;
    uint32_t *pt;
    uint32_t *pt_entry;

    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));
    if (!(error_code & K_NO_PRESENT_MASK)) {
        pt_entry = (uint32_t *)tab_from_addr(fault_addr);
        pt = (uint32_t *)addr_erase_offset(pt_entry);
        if (*pt && (*pt_entry & PG_RESERVED_BIT)) 
            *pt_entry = frame_alloc(PAGE_SIZE) + ((*pt_entry & 0x3FF) | 0x1);
    }
}