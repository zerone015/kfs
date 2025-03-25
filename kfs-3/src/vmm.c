#include "vmm.h"
#include "rbtree.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

static struct k_vspace_allocator kva;

static inline void stack_init(struct free_stack *stack)
{
	stack->top_index = -1;
}

static inline void stack_push(struct free_stack *stack, struct k_vspace *free_node)
{
	stack->top_index++;
	stack->free_nodes[stack->top_index] = free_node;
}

static inline bool stack_is_empty(struct free_stack *stack)
{
	return stack->top_index == -1 ? true : false;
}

static inline struct k_vspace *stack_pop(struct free_stack *stack)
{
	if (stack_is_empty(stack))
		return NULL;
	return stack->free_nodes[stack->top_index--];
}

static inline void __kvs_init(struct k_vspace *kvs)
{
    uint32_t *page_dir;

    page_dir = (uint32_t *)K_PAGE_DIR_BEGIN;
    while (*page_dir)
        page_dir++;
    kvs->addr = addr_from_dir(page_dir);
    while (!*page_dir)
        page_dir++;
    kvs->size = addr_from_dir(page_dir) - kvs->addr;
    init_list_head(&kva.list_head);
    list_add(&kvs->list_head, &kva.list_head);
}

static inline void __free_node_stack_init(struct k_vspace *kvs)
{
    stack_init(&kva.free_stack);
    for (size_t i = 1; i < KVS_MAX_NODE + 1; i++) 
        stack_push(&kva.free_stack, &kvs[i]);
}

static inline void __kvs_allocator_init(uint32_t k_repository)
{
    __kvs_init((struct k_vspace *)k_repository);
    __free_node_stack_init((struct k_vspace *)k_repository);
}

static inline void __vs_reserve(uint32_t v_addr, size_t size)
{
    uint32_t *page_dir;
    size_t i;

    page_dir = (uint32_t *)dir_from_addr(v_addr);
    for (i = 0; i < (size / K_PAGE_SIZE) - 1; i++)
        page_dir[i] = PG_RESERVED_ENTRY | PG_CONTIGUOUS;
    page_dir[i] = PG_RESERVED_ENTRY;
}

static inline size_t __vs_size_with_frame_free(void *addr)
{
    uint32_t *page_dir;
    size_t size;

    size = 0;
    page_dir = (uint32_t *)dir_from_addr(addr);
    do {
        if (*page_dir & PG_PRESENT)
            frame_free(*page_dir & 0xFFC00000, K_PAGE_SIZE);
        size += K_PAGE_SIZE;
    } while (*page_dir++ & PG_CONTIGUOUS);
    return size;
}

static inline void __vs_add_and_merge(uint32_t addr, size_t size)
{
    struct k_vspace *cur;
    struct k_vspace *new;
    
    list_for_each_entry(cur, &kva.list_head, list_head) {
        if (addr < cur->addr)
            break;
    }
    new = stack_pop(&kva.free_stack);
    if (!new)
        panic("Incorrect usage of page_free");
    new->addr = addr;
    new->size = size;
    list_add_tail(&new->list_head, &cur->list_head);

    cur = list_prev_entry(new, list_head);
    if (!list_entry_is_head(cur, &kva.list_head, list_head) && cur->addr + cur->size == new->addr) {
        new->addr = cur->addr;
        new->size += cur->size;
        list_del(&cur->list_head);
        stack_push(&kva.free_stack, cur);
    }
    cur = list_next_entry(new, list_head);
    if (!list_entry_is_head(cur, &kva.list_head, list_head) && new->addr + new->size == cur->addr) {
        new->size += cur->size;
        list_del(&cur->list_head);
        stack_push(&kva.free_stack, cur);
    }
}

void vs_free(void *addr)
{
    size_t size;

    size = __vs_size_with_frame_free(addr);
    __vs_add_and_merge((uint32_t)addr, size);
}

void *vs_alloc(size_t size)
{
    struct k_vspace *cur;
    void *ret;

    ret = NULL;
    list_for_each_entry(cur, &kva.list_head, list_head) {
        if (size <= cur->size) {
            cur->size -= size;
            ret = (void *)(cur->addr + cur->size);
            __vs_reserve((uint32_t)ret, size);
            if (!cur->size) {
                list_del(&cur->list_head);
                stack_push(&kva.free_stack, cur);
            }
            break;
        }
    }
    return ret;
}

uint32_t vm_initmap(uint32_t p_addr, size_t size, uint32_t flags)
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

uint32_t vmm_init(void)
{
    uint32_t frame;
    uint32_t k_repository;

    frame = frame_alloc(K_PAGE_SIZE);
    if (!frame)
        panic("Not enough memory to initialize the virtual memory manager");
    k_repository = vm_initmap(frame, K_PAGE_SIZE, PG_GLOBAL | PG_PS | PG_RDWR | PG_PRESENT);
    __kvs_allocator_init(k_repository);
    return k_repository + KVS_MAX_SIZE + sizeof(struct k_vspace);
}