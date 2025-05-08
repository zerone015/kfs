#include "printk.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_T_PMM_ARRAY 1024 * 1024

static inline int test_bitmap_and_free_count(struct buddy_allocator *bd_alloc)
{
    size_t sum;
    uint32_t last_order_size;
    uint32_t end;

    for (size_t order = 0; order < MAX_ORDER - 1; order++) {
        sum = 0;
        for (uint32_t *bitmap = bd_alloc->orders[order].bitmap; bitmap < bd_alloc->orders[order + 1].bitmap; bitmap++) 
            sum += __builtin_popcount(*bitmap);
        if (bd_alloc->orders[order].free_count != sum)
            return false;
    }
    sum = 0;
    last_order_size = ((((uint32_t)bd_alloc->orders[MAX_ORDER - 1].bitmap - (uint32_t)bd_alloc->orders[MAX_ORDER - 2].bitmap) / 2) + 0x3) & ~0x3;
    end = (uint32_t)bd_alloc->orders[MAX_ORDER - 1].bitmap + last_order_size;
    for (uint32_t *bitmap = bd_alloc->orders[MAX_ORDER - 1].bitmap; (uint32_t)bitmap < end; bitmap++) 
        sum += __builtin_popcount(*bitmap);
    if (bd_alloc->orders[MAX_ORDER - 1].free_count != sum)
        return false;
    return true;
}

static void print_free_count(struct buddy_allocator *bd_alloc)
{
    for (size_t order = 0; order < MAX_ORDER; order++)
        printk("order%u free_count: %u\n", order, bd_alloc->orders[order].free_count);
}

/* 
 * 테스트하기전에 boot.s에서 스택의 크기를 5MB로 설정해야 합니다.
 */
void test_page_allocator(multiboot_memory_map_t *mmap, size_t mmap_count, struct buddy_allocator *bd_alloc)
{
    uintptr_t addrs[MAX_T_PMM_ARRAY];

    // print_free_count(bd_alloc);
    
    printk("PMM test case 1 (only 4kb page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 2 (only 8kb page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 1);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 1);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 1);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 1);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 3 (only 16kb page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 2);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 2);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 2);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 2);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 4 (only 32kb page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 3);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 3);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 3);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 3);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 5 (only 64kb page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 4);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 4);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 4);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 4);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 6-1 (only 128kb page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 5);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 5);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 5);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 5);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 6-2 (only 256KB page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 6);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 6);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 6);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 6);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 6-3 (only 512KB page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 7);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 7);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 7);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 7);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n"); 
    
    printk("PMM test case 6-4 (only 1MB page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 8);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 8);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 8);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 8);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n"); 

    printk("PMM test case 6-5 (only 2MB page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 9);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 9);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 9);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 9);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n"); 

    printk("PMM test case 6-6 (only 4MB page): ");
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 10);
    for (size_t i = 1; i <= MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[MAX_T_PMM_ARRAY - i], PAGE_SIZE << 10);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        addrs[i] = alloc_pages(PAGE_SIZE << 10);
    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) 
        free_pages(addrs[i], PAGE_SIZE << 10);
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 7-1 (multiple page sizes): ");
    for (size_t i = 0; i <= MAX_T_PMM_ARRAY - MAX_ORDER; i += MAX_ORDER) {
        for (size_t order = 0; order < MAX_ORDER; order++)
            addrs[i + order] = alloc_pages(PAGE_SIZE << order);
    }
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");
    printk("PMM test case 7-2 (multiple page sizes): ");
    for (size_t i = 0; i <= MAX_T_PMM_ARRAY - MAX_ORDER; i += MAX_ORDER) {
        for (size_t order = 0; order < MAX_ORDER; order++)
            free_pages(addrs[i + order], PAGE_SIZE << order);
    }
    if (test_bitmap_and_free_count(bd_alloc))
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    {  
        size_t addrs_len;
        size_t flag;
        size_t is_ok = 1;

        printk("PMM test case 8 (checking if all return addrs are valid): ");
        for (size_t block_size = PAGE_SIZE; block_size <= (PAGE_SIZE << (MAX_ORDER - 1)) && is_ok; block_size <<= 1) {
            for (addrs_len = 0; addrs_len < MAX_T_PMM_ARRAY; addrs_len++){
                addrs[addrs_len] = alloc_pages(block_size);
                if (!addrs[addrs_len])
                    break;
            }
            for (size_t i = 0; i < addrs_len; i++) {
                flag = 0;
                for (size_t j = 0; j < mmap_count; j++) {
                    if (mmap[j].type == MULTIBOOT_MEMORY_AVAILABLE && mmap[j].addr < MAX_RAM_SIZE) {
                        if ((uintptr_t)addrs[i] >= mmap[j].addr && (uintptr_t)addrs[i] < mmap[j].addr + mmap[j].len)
                            flag = 1;
                    }
                }
                if (!flag) {
                    is_ok = 0;
                    break;
                }
            }
            for (size_t i = 0; i < addrs_len; i++)
                free_pages(addrs[i], block_size);
        }
        if (is_ok)
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
    }

    {
        printk("PMM test case 9-1 (no available pages): "); 
        size_t size = 0;
        for (size_t order = 0; order < MAX_ORDER; order++) 
            size += bd_alloc->orders[order].free_count << (PAGE_SHIFT + order);
        for (size_t i = 0; i < (size / PAGE_SIZE); i++)
            addrs[i] = alloc_pages(PAGE_SIZE);
        if (!alloc_pages(PAGE_SIZE))
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
        printk("PMM test case 9-2 (no available pages): "); 
        if (test_bitmap_and_free_count(bd_alloc))
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
        printk("PMM test case 9-3 (no available pages): "); 
        for (size_t i = 0; i < (size / PAGE_SIZE); i++) 
            free_pages(addrs[i], PAGE_SIZE);
        if (test_bitmap_and_free_count(bd_alloc))
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
    }
    printk(KERN_DEBUG "PMM test completed\n");
    // print_free_count(bd_alloc);
}