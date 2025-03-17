#include "vmm.h"
#include "rbtree.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

struct rb_node *rb_addr_root;
struct rb_node *rb_size_root;


uint32_t page_map(uint32_t p_addr, size_t size, uint32_t mode)
{
    uint32_t *k_page_table;

    k_page_table = (uint32_t *)tab_from_addr(VIRT_KERNEL_END);
    while (*k_page_table)
        k_page_table++;
    for (size_t i = 0; i < size / PAGE_SIZE; i++) {
        k_page_table[i] = p_addr | mode;
        p_addr += PAGE_SIZE; 
    }
    return addr_from_tab(k_page_table);
}

void vmm_init(void)
{
    uint32_t frame;
    uint32_t page;

    frame = frame_alloc(PAGE_SIZE);
    if (!frame)
        panic("Not enough memory to initialize the virtual memory manager");
    page = page_map(frame, PAGE_SIZE, PAGE_TAB_GLOBAL | PAGE_TAB_RDWR);
}