#include "printk.h"
#include "vmm.h"
#include "list.h"

#define MAX_T_VMM_ARRAY 256

void test_virtual_address_allocator(struct list_head *vblocks)
{
    uintptr_t addrs[MAX_T_VMM_ARRAY];
    struct kernel_vblock *cur;
    uintptr_t initial_addr;
    size_t initial_size;

    cur = list_first_entry(vblocks, typeof(*cur), list_head);
    initial_addr = cur->base;
    initial_size = cur->size;

    printk("VMM test case 1 (a single 4MB virtual block): ");
    {

        addrs[0] = (uintptr_t)vb_alloc(K_PAGE_SIZE);
        if (!(addrs[0] >= initial_addr && addrs[0] < initial_addr + initial_size && addrs[0] % K_PAGE_SIZE == 0)) {
            printk(KERN_ERR "Failed\n");
            return;
        }
        cur = list_first_entry(vblocks, typeof(*cur), list_head);
        if (addrs[0] >= cur->base && addrs[0] < cur->base + cur->size) {
            printk(KERN_ERR "Failed\n");
            return;
        }

        char *page_fault_handler_test = (char *)addrs[0];
        *page_fault_handler_test = 'T';
        if (*page_fault_handler_test != 'T') {
            printk(KERN_ERR "Failed\n");
            return;
        }
        
        vb_free((void *)addrs[0]);
        cur = list_first_entry(vblocks, typeof(*cur), list_head);
        if ((initial_addr != cur->base || initial_size != cur->size) || list_count_nodes(vblocks) != 1) {
            printk(KERN_ERR "Failed\n");
            return;
        }
        printk(KERN_DEBUG "OK\n");
    }

    printk("VMM test case 2 (multiple 4MB virtual blocks): ");
    {
        for (size_t i = 0; i < initial_size / K_PAGE_SIZE; i++) {
            addrs[i] = (uintptr_t)vb_alloc(K_PAGE_SIZE);
            
            if (!(addrs[i] >= initial_addr && addrs[i] < initial_addr + initial_size && addrs[i] % K_PAGE_SIZE == 0)) {
                printk(KERN_ERR "Failed\n");
                return;
            }

            cur = list_first_entry(vblocks, typeof(*cur), list_head);
            if (addrs[i] >= cur->base && addrs[i] < cur->base + cur->size) {
                printk(KERN_ERR "Failed\n");
                return;
            }
    
            char *page_fault_handler_test = (char *)addrs[i];
            *page_fault_handler_test = 'T';
            if (*page_fault_handler_test != 'T') {
                printk(KERN_ERR "Failed\n");
                return;
            }
        }
        if (vb_alloc(K_PAGE_SIZE) != VB_ALLOC_FAILED) {
            printk(KERN_ERR "Failed\n");
            return;
        }

        for (size_t i = 0; i < initial_size / K_PAGE_SIZE; i++)
            vb_free((void *)addrs[i]);
        cur = list_first_entry(vblocks, typeof(*cur), list_head);
        if ((initial_addr != cur->base || initial_size != cur->size) || list_count_nodes(vblocks) != 1) {
            printk(KERN_ERR "Failed\n");
            return;
        }      
        printk(KERN_DEBUG "OK\n");
    }
    printk("VMM test case 3 (multiple size virtual blocks): ");
    {
        size_t i;
        uintptr_t tmp;
        size_t size = K_PAGE_SIZE;

        for (i = 0; (addrs[i] = (uintptr_t)vb_alloc(size)) != VB_ALLOC_FAILED; i++) {
            for (size_t j = 0; j < size / K_PAGE_SIZE; j++) {
                tmp = addrs[i] + (K_PAGE_SIZE * j);
                if (!(tmp >= initial_addr && tmp < initial_addr + initial_size && tmp % K_PAGE_SIZE == 0)) {
                    printk(KERN_ERR "Failed\n");
                    return;
                }
            }

            cur = list_first_entry(vblocks, typeof(*cur), list_head);
            for (size_t j = 0; j < size / K_PAGE_SIZE; j++) {
                tmp = addrs[i] + (K_PAGE_SIZE * j);
                if (tmp >= cur->base && tmp < cur->base + cur->size) {
                    printk(KERN_ERR "Failed\n");
                    return;
                }
            }
        
            for (size_t j = 0; j < size / K_PAGE_SIZE; j++) {
                tmp = addrs[i] + (K_PAGE_SIZE * j);
                char *page_fault_handler_test = (char *)tmp;
                *page_fault_handler_test = 'T';
                if (*page_fault_handler_test != 'T') {
                    printk(KERN_ERR "Failed\n");
                    return;
                }
            }
            size += K_PAGE_SIZE;
        }
        for (size_t j = 0; j < i; j++) {
            if (j % 2 == 0)
                vb_free((void *)addrs[j]);
        }
        for (size_t j = 0; j < i; j++) {
            if (j % 2 == 1)
                vb_free((void *)addrs[j]);
        }
        addrs[0] = (uintptr_t)vb_alloc(initial_size);
        if (addrs[0] == VB_ALLOC_FAILED) {
            printk(KERN_ERR "Failed\n");
            return;
        }
        vb_free((void *)addrs[0]);

        cur = list_first_entry(vblocks, typeof(*cur), list_head);
        if ((initial_addr != cur->base || initial_size != cur->size) || list_count_nodes(vblocks) != 1) {
            printk(KERN_ERR "Failed\n");
            return;
        }
        printk(KERN_DEBUG "OK\n");
    }
    printk(KERN_DEBUG "VMM test completed\n");
}