#include "printk.h"
#include "pmm.h"
#include <stdint.h>
#include <stdbool.h>

extern struct meminfo meminfo;

#define MAX_T_PMM_ARRAY 512

#define TEST_OK()    printk(KERN_DEBUG "OK\n")
#define TEST_FAIL()  printk(KERN_ERR "Failed\n")

static size_t count_free_pages(struct buddy_allocator *bd_alloc)
{
    struct buddy_order *bo;
    size_t ret;
    size_t bpages;
    size_t count;
    
    ret = 0;
    for (size_t order = 0; order < MAX_ORDER; order++) {
        bo = &bd_alloc->orders[order];

        bpages = block_pages(order);
        count = list_count_nodes(&bo->free_list);

        ret += count * bpages;
    }
    return ret;
}

static bool check_free_consistency(struct buddy_allocator *bd_alloc)
{
    return count_free_pages(bd_alloc) == meminfo.usable_pages;
}

static void test_single_order(size_t order)
{
    size_t addrs[MAX_T_PMM_ARRAY];
    size_t count;
    size_t block_size;
    size_t a;

    block_size = PAGE_SIZE << order;
    count = 0;

    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) {
        a = alloc_pages(block_size);
        if (a == PAGE_NONE)
            break;
        addrs[count++] = a;
    } 

    while (count--)
        free_pages(addrs[count], block_size);
}

static bool test_mixed(void)
{
    size_t addrs[MAX_T_PMM_ARRAY];
    size_t sizes[MAX_T_PMM_ARRAY];
    size_t count = 0;
    size_t block_size;
    size_t a;

    for (;;) {
        for (size_t o = 0; o < MAX_ORDER; o++) {

            if (count >= MAX_T_PMM_ARRAY)
                goto free_all;

            block_size = PAGE_SIZE << o; 
            a = alloc_pages(block_size);

            if (a == PAGE_NONE)
                goto free_all;

            addrs[count] = a;
            sizes[count] = block_size;
            count++;
        }
    }

free_all:
    for (size_t i = 0; i < count; i++) {
        free_pages(addrs[i], sizes[i]);
    }

    return true;
}

static bool test_addr_validity(struct mmap_buffer *mmap)
{
    size_t addrs[MAX_T_PMM_ARRAY];
    size_t block_size, count;
    size_t a;
    bool valid, ok;
    multiboot_memory_map_t *r;

    block_size = PAGE_SIZE;
    count = 0;

    for (size_t i = 0; i < MAX_T_PMM_ARRAY; i++) {
        a = alloc_pages(block_size);
        if (a == PAGE_NONE)
            break;

        addrs[count++] = a;
    }

    valid = true;

    for (size_t i = 0; i < count; i++) {
        a = addrs[i];
        ok = false;

        for (size_t j = 0; j < mmap->count; j++) {
            r = &mmap->regions[j];

            if (r->type == MULTIBOOT_MEMORY_AVAILABLE &&
                is_aligned(r->addr, PAGE_SIZE) &&
                a >= r->addr &&
                a < r->addr + r->len) {
                ok = true;
                break;
            }
        }

        if (!ok) {
            valid = false;
            break;
        }
    }

    while (count--)
        free_pages(addrs[count], block_size);

    return valid;
}

void test_pmm(struct buddy_allocator *bd_alloc, struct mmap_buffer *mmap)
{
    bool ok1;
    bool ok2;

    printk("PMM test: single-order tests\n");

    for (size_t order = 0; order < MAX_ORDER; order++) {
        printk(" order %u: ", order);

        test_single_order(order);

        if (check_free_consistency(bd_alloc))
            TEST_OK();
        else
            TEST_FAIL();
    }

    printk("PMM test: mixed alloc/free: ");

    ok1 = test_mixed();
    ok2 = check_free_consistency(bd_alloc);

    if (ok1 && ok2)
        TEST_OK();
    else
        TEST_FAIL();

    printk("PMM test: address validity: ");

    if (test_addr_validity(mmap) &&
        check_free_consistency(bd_alloc))
        TEST_OK();
    else
        TEST_FAIL();

    printk("PMM tests completed.\n");
}