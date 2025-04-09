#include "proc.h"
#include "panic.h"
#include "pmm.h"
#include "hmm.h"
#include "utils.h"

static uint16_t *page_ref;

static inline void __page_ref_init(void)
{
    size_t page_ref_size;

    page_ref_size = ram_size / PAGE_SIZE * sizeof(uint16_t);
    page_ref = kmalloc(page_ref_size);
    if (!page_ref)
        do_panic("scheduler_init failed");
    memset(page_ref, 0, page_ref_size);
}

void proc_init(void)
{
    __page_ref_init();
}