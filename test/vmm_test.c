#include "printk.h"
#include "vmm.h"
#include "list.h"

#define MAX_T_VMM_ARRAY 256

#define TEST_FAIL() do { printk(KERN_ERR "Failed\n"); return; } while (0)
#define TEST_OK()   printk(KERN_DEBUG "OK\n")

static inline bool addr_in_range(uintptr_t addr, uintptr_t base, size_t size)
{
    return (addr >= base && addr < base + size);
}

static inline bool addr_is_aligned(uintptr_t addr)
{
    return (addr % PAGE_LARGE_SIZE == 0);
}

static inline uintptr_t test_page_fault(uintptr_t addr)
{
    char *p = (char *)addr;
    *p = 'T';
    return (*p == 'T');
}

static void test_case_single(struct list_head *vblocks,
                             uintptr_t initial_addr,
                             size_t initial_size)
{
    struct k_vblock *cur;
    uintptr_t addr = (uintptr_t)vb_alloc(PAGE_LARGE_SIZE);

    if (!addr || !addr_in_range(addr, initial_addr, initial_size) ||
        !addr_is_aligned(addr))
        TEST_FAIL();

    cur = list_first_entry(vblocks, typeof(*cur), list_head);
    if (addr_in_range(addr, cur->base, cur->size))
        TEST_FAIL();

    if (!test_page_fault(addr))
        TEST_FAIL();

    vb_free((void *)addr);
    cur = list_first_entry(vblocks, typeof(*cur), list_head);

    if (cur->base != initial_addr || cur->size != initial_size ||
        list_count_nodes(vblocks) != 1)
        TEST_FAIL();

    TEST_OK();
}

static void test_case_multiple(struct list_head *vblocks,
                               uintptr_t initial_addr,
                               size_t initial_size)
{
    uintptr_t addrs[MAX_T_VMM_ARRAY];
    struct k_vblock *cur;
    size_t blocks = initial_size / PAGE_LARGE_SIZE;

    for (size_t i = 0; i < blocks; i++) {
        addrs[i] = (uintptr_t)vb_alloc(PAGE_LARGE_SIZE);

        if (!addrs[i] ||
            !addr_in_range(addrs[i], initial_addr, initial_size) ||
            !addr_is_aligned(addrs[i]))
            TEST_FAIL();

        cur = list_first_entry(vblocks, typeof(*cur), list_head);
        if (addr_in_range(addrs[i], cur->base, cur->size))
            TEST_FAIL();

        if (!test_page_fault(addrs[i]))
            TEST_FAIL();
    }

    if (vb_alloc(PAGE_LARGE_SIZE))
        TEST_FAIL();

    for (size_t i = 0; i < blocks; i++)
        vb_free((void *)addrs[i]);

    cur = list_first_entry(vblocks, typeof(*cur), list_head);
    if (cur->base != initial_addr || cur->size != initial_size ||
        list_count_nodes(vblocks) != 1)
        TEST_FAIL();

    TEST_OK();
}

static void test_case_various(struct list_head *vblocks,
                              uintptr_t initial_addr,
                              size_t initial_size)
{
    uintptr_t addrs[MAX_T_VMM_ARRAY];
    size_t i = 0, size = PAGE_LARGE_SIZE;
    uintptr_t tmp;
    struct k_vblock *cur;

    while ((addrs[i] = (uintptr_t)vb_alloc(size))) {

        for (size_t j = 0; j < size / PAGE_LARGE_SIZE; j++) {
            tmp = addrs[i] + PAGE_LARGE_SIZE * j;

            if (!addr_in_range(tmp, initial_addr, initial_size) ||
                !addr_is_aligned(tmp))
                TEST_FAIL();
        }

        cur = list_first_entry(vblocks, typeof(*cur), list_head);

        for (size_t j = 0; j < size / PAGE_LARGE_SIZE; j++) {
            tmp = addrs[i] + PAGE_LARGE_SIZE * j;
            if (addr_in_range(tmp, cur->base, cur->size))
                TEST_FAIL();
        }

        for (size_t j = 0; j < size / PAGE_LARGE_SIZE; j++) {
            tmp = addrs[i] + PAGE_LARGE_SIZE * j;
            if (!test_page_fault(tmp))
                TEST_FAIL();
        }

        i++;
        size += PAGE_LARGE_SIZE;
    }

    for (size_t j = 0; j < i; j++)
        if ((j % 2) == 0)
            vb_free((void *)addrs[j]);

    for (size_t j = 0; j < i; j++)
        if ((j % 2) == 1)
            vb_free((void *)addrs[j]);

    addrs[0] = (uintptr_t)vb_alloc(initial_size);
    if (!addrs[0])
        TEST_FAIL();

    vb_free((void *)addrs[0]);

    cur = list_first_entry(vblocks, typeof(*cur), list_head);
    if (cur->base != initial_addr || cur->size != initial_size ||
        list_count_nodes(vblocks) != 1)
        TEST_FAIL();

    TEST_OK();
}

/*
 * During this test, all signal–handling logic in the page-fault handler
 * must be disabled, as the test intentionally triggers page faults.
 */
void test_vmm(struct list_head *vblocks)
{
    struct k_vblock *cur = list_first_entry(vblocks, typeof(*cur), list_head);
    uintptr_t initial_addr = cur->base;
    size_t initial_size = cur->size;

    printk("VMM test case 1 (single 4MB block): ");
    test_case_single(vblocks, initial_addr, initial_size);

    printk("VMM test case 2 (multiple 4MB blocks): ");
    test_case_multiple(vblocks, initial_addr, initial_size);

    printk("VMM test case 3 (various sizes): ");
    test_case_various(vblocks, initial_addr, initial_size);

    printk("VMM test completed\n");
}