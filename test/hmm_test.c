#include "kmalloc.h"
#include "printk.h"
#include "utils.h"

#define MAX_LIMIT       (128 * 1024 * 1024)

#define TEST_FAIL() 	do { printk(KERN_ERR "Failed\n"); return; } while (0)
#define TEST_OK()   	printk(KERN_DEBUG "OK\n")

static const size_t test_sizes[] = {
    8, 16, 24, 32, 64,
    128, 256, 512,
    1024, 2048, 4096,
    8192,
};

static const size_t nr_test_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

static bool validate_pattern(void *ptr, char pattern, size_t size)
{
    char *p = ptr;
    for (size_t i = 0; i < size; i++) {
        if (p[i] != pattern)
            return false;
    }
    return true;
}

/*
 * During this test, all signal–handling logic in the page-fault handler
 * must be disabled, as the test intentionally triggers page faults.
 */
void test_malloc(void)
{
    void **addrs;
    void *tmp;

    printk("--malloc test--\n");

    tmp = kmalloc(PAGE_LARGE_SIZE * 100);
    if (!tmp)
        TEST_FAIL();
    kfree(tmp);

    for (size_t sz_idx = 0; sz_idx < nr_test_sizes; sz_idx++) {

        size_t alloc_size = test_sizes[sz_idx];
        size_t count = MAX_LIMIT / alloc_size;

        printk(" [size=%u count=%u]: ", alloc_size, count);

        for (size_t loop = 0; loop < 3; loop++) {

            addrs = kmalloc(count * sizeof(void *));
            if (!addrs || ksize(addrs) != count * sizeof(void *))
                TEST_FAIL();

            for (size_t i = 0; i < count; i++) {
                addrs[i] = kmalloc(alloc_size);

                if (!addrs[i])
                    TEST_FAIL();

                memset(addrs[i],
                       (i % 2 == 0) ? 'a' : 'b',
                       alloc_size);
            }

            for (size_t i = 0; i < count; i++) {
                char expected = (i % 2 == 0) ? 'a' : 'b';
                if (!validate_pattern(addrs[i], expected, alloc_size))
                    TEST_FAIL();
            }

            for (size_t i = 0; i < count; i++)
                if ((i % 2) == 0)
                    kfree(addrs[i]);

            for (size_t i = 0; i < count; i++)
                if ((i % 2) == 1)
                    kfree(addrs[i]);

            kfree(addrs);
        }
        TEST_OK();
    }
    printk("malloc test completed\n");
}
