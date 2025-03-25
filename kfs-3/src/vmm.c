#include "vmm.h"
#include "rbtree.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

struct k_vspace_manager kvm;

uint32_t page_map(uint32_t p_addr, size_t size, uint32_t flags)
{
    uint32_t *page_dir;

    page_dir = (uint32_t *)K_PAGE_DIR_BEGIN;
    while (*page_dir)
        page_dir++;
    p_addr = k_addr_erase_offset(p_addr);
    size = align_kpage(size);
    for (size_t i = 0; i < (size / K_PAGE_SIZE); i++) {
        page_dir[i] = p_addr | flags;
        p_addr += K_PAGE_SIZE;
    }
    return addr_from_dir(page_dir);
}

static void kvs_init(struct k_vspace *kvs)
{
    uint32_t *page_dir;

    init_list_head(&kvm.list_head);
    page_dir = (uint32_t *)K_PAGE_DIR_BEGIN;
    while (*page_dir)
        page_dir++;
    kvs->addr = addr_from_dir(page_dir);
    while (!*page_dir)
        page_dir++;
    kvs->size = addr_from_dir(page_dir) - kvs->addr;
    list_add(&kvs->list_head, &kvm.list_head);

    stack_init(&kvm.free_stack);
    for (size_t i = 1; i < KVS_MAX_NODE + 1; i++) 
        stack_push(&kvm.free_stack, &kvs[i]);
}

void vspace_reserve(uint32_t v_addr, size_t size)
{
    uint32_t *page_dir;
    size_t i;

    page_dir = (uint32_t *)dir_from_addr(v_addr);
    for (i = 0; i < (size / K_PAGE_SIZE) - 1; i++)
        page_dir[i] = PG_RESERVED_ENTRY | PG_CONTIGUOUS;
    page_dir[i] = PG_RESERVED_ENTRY;
}

void *vb_alloc(size_t size)
{
    struct k_vspace *cur;
    void *ret = NULL;

    list_for_each_entry(cur, &kvm.list_head, list_head) {
        if (size <= cur->size) {
            cur->size -= size;
            ret = (void *)(cur->addr + cur->size);
            vspace_reserve((uint32_t)ret, size);
            if (!cur->size) {
                list_del(&cur->list_head);
                stack_push(&kvm.free_stack, cur);
            }
            break;
        }
    }
    return ret;
}

void vb_free(void *addr)
{
    struct k_vspace *cur;
    struct k_vspace *new;
    uint32_t *page_dir;
    size_t size = 0;

    page_dir = (uint32_t *)dir_from_addr(addr);
    do {
        if (*page_dir & PG_PRESENT)
            frame_free(*page_dir & 0xFFC00000, K_PAGE_SIZE);
        size += K_PAGE_SIZE;
    } while (*page_dir++ & PG_CONTIGUOUS);
    list_for_each_entry(cur, &kvm.list_head, list_head) {
        if ((uint32_t)addr < cur->addr)
            break;
    }
    new = stack_pop(&kvm.free_stack);
    if (!new)
        panic("Incorrect usage of page_free");
    new->addr = (uint32_t)addr;
    new->size = size;
    list_add_tail(&new->list_head, &cur->list_head);
    cur = list_prev_entry(new, list_head);
    if (!list_entry_is_head(cur, &kvm.list_head, list_head) && cur->addr + cur->size == new->addr) {
        new->addr = cur->addr;
        new->size += cur->size;
        list_del(&cur->list_head);
        stack_push(&kvm.free_stack, cur);
    }
    cur = list_next_entry(new, list_head);
    if (!list_entry_is_head(cur, &kvm.list_head, list_head) && new->addr + new->size == cur->addr) {
        new->size += cur->size;
        list_del(&cur->list_head);
        stack_push(&kvm.free_stack, cur);
    }
}

void vmm_init(void)
{
    uint32_t free_frame;
    uint32_t free_page;

    free_frame = frame_alloc(K_PAGE_SIZE);
    if (!free_frame)
        panic("Not enough memory to initialize the virtual memory manager");
    free_page = page_map(free_frame, K_PAGE_SIZE, PG_GLOBAL | PG_PS | PG_RDWR | PG_PRESENT);
    kvs_init((struct k_vspace *)free_page);
    free_page += KVS_MAX_SIZE + sizeof(struct k_vspace); //free_page 남은 메모리 힙에서 써야함
}