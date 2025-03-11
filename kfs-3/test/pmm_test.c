#include "printk.h"
#include "pmm.h"
#include <stdint.h>

#define T_ARRAY_SIZE 1024 * 256

static inline int test_bitmap_and_free_count(void)
{
    size_t sum;
    uint32_t last_order_size;
    uint32_t end;

    for (size_t order = 0; order < MAX_ORDER - 1; order++) {
        sum = 0;
        for (uint32_t *bitmap = buddy_allocator.orders[order].bitmap; bitmap < buddy_allocator.orders[order + 1].bitmap; bitmap++) 
            sum += __builtin_popcount(*bitmap);
        if (buddy_allocator.orders[order].free_count != sum)
            return 0;
    }
    sum = 0;
    last_order_size = ((((uint32_t)buddy_allocator.orders[MAX_ORDER - 1].bitmap - (uint32_t)buddy_allocator.orders[MAX_ORDER - 2].bitmap) / 2) + 0x3) & ~0x3;
    end = (uint32_t)buddy_allocator.orders[MAX_ORDER - 1].bitmap + last_order_size;
    for (uint32_t *bitmap = buddy_allocator.orders[MAX_ORDER - 1].bitmap; (uint32_t)bitmap < end; bitmap++) 
        sum += __builtin_popcount(*bitmap);
    if (buddy_allocator.orders[MAX_ORDER - 1].free_count != sum)
        return 0;
    return 1;
}

/* 이 테스트는 최대 1GB 크기의 RAM을 지원한다.
 * 테스트하기전에 boot.s에서 스택의 크기를 2MB까지 늘려야 한다.
 * 그리고 T_ARRAY_SIZE를 RAM 크기에 맞게 조정해야한다. 
 */
void test_frame_allocator(multiboot_memory_map_t *mmap, size_t mmap_count)
{
    void* addrs[T_ARRAY_SIZE];

    for (size_t order = 0; order < MAX_ORDER; order++)
        printk("order%u free_count: %u\n", order, buddy_allocator.orders[order].free_count);

    printk("PMM test case 1 (only 4kb page): ");
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE);
    for (size_t i = 1; i <= T_ARRAY_SIZE; i++) 
        frame_free(addrs[T_ARRAY_SIZE - i], PAGE_SIZE);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        frame_free(addrs[i], PAGE_SIZE);
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 2 (only 8kb page): ");
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 2);
    for (size_t i = 1; i <= T_ARRAY_SIZE; i++) 
        frame_free(addrs[T_ARRAY_SIZE - i], PAGE_SIZE * 2);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 2);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        frame_free(addrs[i], PAGE_SIZE * 2);
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 3 (only 16kb page): ");
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 4);
    for (size_t i = 1; i <= T_ARRAY_SIZE; i++) 
        frame_free(addrs[T_ARRAY_SIZE - i], PAGE_SIZE * 4);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 4);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        frame_free(addrs[i], PAGE_SIZE * 4);
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 4 (only 32kb page): ");
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 8);
    for (size_t i = 1; i <= T_ARRAY_SIZE; i++) 
        frame_free(addrs[T_ARRAY_SIZE - i], PAGE_SIZE * 8);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 8);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        frame_free(addrs[i], PAGE_SIZE * 8);
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 5 (only 64kb page): ");
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 16);
    for (size_t i = 1; i <= T_ARRAY_SIZE; i++) 
        frame_free(addrs[T_ARRAY_SIZE - i], PAGE_SIZE * 16);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 16);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        frame_free(addrs[i], PAGE_SIZE * 16);
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 6 (only 128kb page): ");
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 32);
    for (size_t i = 1; i <= T_ARRAY_SIZE; i++) 
        frame_free(addrs[T_ARRAY_SIZE - i], PAGE_SIZE * 32);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        addrs[i] = frame_alloc(PAGE_SIZE * 32);
    for (size_t i = 0; i < T_ARRAY_SIZE; i++) 
        frame_free(addrs[i], PAGE_SIZE * 32);
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    printk("PMM test case 7-1 (multiple page sizes): ");
    for (size_t i = 0; i <= T_ARRAY_SIZE - MAX_ORDER; i += MAX_ORDER) {
        for (size_t order = 0; order < MAX_ORDER; order++)
            addrs[i + order] = frame_alloc(PAGE_SIZE << order);
    }
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");
    printk("PMM test case 7-2 (multiple page sizes): ");
    for (size_t i = 0; i <= T_ARRAY_SIZE - MAX_ORDER; i += MAX_ORDER) {
        for (size_t order = 0; order < MAX_ORDER; order++)
            frame_free(addrs[i + order], PAGE_SIZE << order);
    }
    if (test_bitmap_and_free_count())
        printk(KERN_DEBUG "OK\n");
    else
        printk(KERN_ERR "Failed\n");

    {  
        size_t addrs_len;
        size_t flag;
        size_t is_ok = 1;

        printk("PMM test case 8 (checking if all return addrs are valid): ");
        for (size_t block_size = PAGE_SIZE; block_size <= (PAGE_SIZE << (MAX_ORDER - 1)) && is_ok; block_size <<= 1) {
            for (addrs_len = 0; addrs_len < T_ARRAY_SIZE; addrs_len++){
                addrs[addrs_len] = frame_alloc(block_size);
                if (!addrs[addrs_len])
                    break;
            }
            for (size_t i = 0; i < addrs_len; i++) {
                flag = 0;
                for (size_t j = 0; j < mmap_count; j++) {
                    if (mmap[j].type == MULTIBOOT_MEMORY_AVAILABLE && mmap[j].addr < MAX_RAM_SIZE) {
                        if ((uint32_t)addrs[i] >= mmap[j].addr && (uint32_t)addrs[i] < mmap[j].addr + mmap[j].len)
                            flag = 1;
                    }
                }
                if (!flag) {
                    is_ok = 0;
                    break;
                }
            }
            for (size_t i = 0; i < addrs_len; i++)
                frame_free(addrs[i], block_size);
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
            size += buddy_allocator.orders[order].free_count << (PAGE_SHIFT + order);
        for (size_t i = 0; i < (size / PAGE_SIZE); i++)
            addrs[i] = frame_alloc(PAGE_SIZE);
        if (!frame_alloc(PAGE_SIZE))
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
        printk("PMM test case 9-2 (no available pages): "); 
        if (test_bitmap_and_free_count())
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
        printk("PMM test case 9-3 (no available pages): "); 
        for (size_t i = 0; i < (size / PAGE_SIZE); i++) 
            frame_free(addrs[i], PAGE_SIZE);
        if (test_bitmap_and_free_count())
            printk(KERN_DEBUG "OK\n");
        else
            printk(KERN_ERR "Failed\n");
    }
    
    for (size_t order = 0; order < MAX_ORDER; order++)
        printk("order%u free_count: %u\n", order, buddy_allocator.orders[order].free_count);
}