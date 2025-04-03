#include "hmm.h"
#include "printk.h"
#include "utils.h"

#define MAX_T_MALLOC_ARRAY 	1024 * 1024
#define T_MALLOC_SIZE		500

void test_malloc(void)
{
    void **addrs;
	void *tmp;

    printk("malloc test: ");
	tmp = kmalloc(K_PAGE_SIZE * 100);
	if (!tmp) {
		printk(KERN_ERR "Failed\n");
		return;
	}
	kfree(tmp);
	for (size_t loop_count = 0; loop_count < 1; loop_count++) {
		addrs = kmalloc(MAX_T_MALLOC_ARRAY * sizeof(void *));
		if (!addrs && ksize(addrs) != (MAX_T_MALLOC_ARRAY * sizeof(void *))) {
			printk(KERN_ERR "Failed\n");
			return;
		}
		for (size_t i = 0; i < MAX_T_MALLOC_ARRAY; i++) {
			addrs[i] = kmalloc(T_MALLOC_SIZE);
			if (!addrs[i]) {
				printk(KERN_ERR "Failed\n");
				return;
			}
			if (i % 2 == 0) 
				memset(addrs[i], 'a', T_MALLOC_SIZE);
			else
				memset(addrs[i], 'b', T_MALLOC_SIZE);
		}
		for (size_t i = 0; i < MAX_T_MALLOC_ARRAY; i++) {
			if (i % 2 == 0) {
				for (size_t j = 0; j < T_MALLOC_SIZE; j++) {
					if (((char *)addrs[i])[j] != 'a') {
						printk(KERN_ERR "Failed\n");
						return;
					}
				}
			}
			else {
				for (size_t j = 0; j < T_MALLOC_SIZE; j++) {
					if (((char *)addrs[i])[j] != 'b') {
						printk(KERN_ERR "Failed\n");
						return;
					}
				}
			}
		}
		for (size_t i = 0; i < MAX_T_MALLOC_ARRAY; i++) {
			if (i % 2 == 0)
				kfree(addrs[i]);
		}
		for (size_t i = 0; i < MAX_T_MALLOC_ARRAY; i++) {
			if (i % 2 == 1)
				kfree(addrs[i]);
		}
		kfree(addrs);
	}
    
    printk(KERN_DEBUG "OK\n");
    printk(KERN_DEBUG "malloc test completed\n");
}