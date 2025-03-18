#include "page_fault_handle.h"
#include "pmm.h"
#include "paging.h"

void page_fault_handle(uint32_t error_code) 
{
    uint32_t faulting_address;

    asm volatile ("mov %%cr2, %0" : "=r" (faulting_address));
    if (!(error_code & K_MMAP_MASK)) {
        
    }
}