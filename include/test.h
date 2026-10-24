#ifndef _TEST_H
#define _TEST_H

#include <stddef.h>
#include <stdint.h>
#include "printk.h"

#define TEST_DONE_MARKER    "KFS_TEST_DONE\n"

struct buddy_allocator;
struct mmap_buffer;
struct list_head;

void test_pmm(struct buddy_allocator *bd_alloc, struct mmap_buffer *mmap);
void test_vmm(struct list_head *vblocks);
void test_malloc(void);
void panic_test1(void);
void panic_test2(void);
void test_exec_hook(void);

static inline void __attribute__((noreturn)) test_done(void)
{
    printk(TEST_DONE_MARKER);
    __asm__ volatile ("cli");
    while (1)
        __asm__ volatile ("hlt");
}

#endif