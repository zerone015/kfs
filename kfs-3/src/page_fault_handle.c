#include "page_fault_handle.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

void page_fault_handle(uint32_t error_code) 
{
    uint32_t fault_addr;
    uint32_t *page_dir;

    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));
    if (!(error_code & K_NO_PRESENT_MASK)) {
        page_dir = (uint32_t *)dir_from_addr(fault_addr);
        if (*page_dir & PG_RESERVED) {
            *page_dir = alloc_pages(K_PAGE_SIZE) + ((*page_dir & 0x17FF) | 0x1);
            return;
        }
    }
    panic("page fault"); //임시 조치
}