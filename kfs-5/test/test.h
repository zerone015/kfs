#ifndef _TEST_H
#define _TEST_H

#include "multiboot.h"
#include <stddef.h>

extern void test_malloc(void);
extern void panic_test1(void);
extern void panic_test2(void);
extern void test_page_allocator(multiboot_memory_map_t *mmap, size_t mmap_count, struct buddy_allocator *bd_alloc);
extern void test_virtual_address_allocator(struct k_vblock_allocator *kvs_alloc);

#endif