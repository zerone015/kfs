#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

static struct k_vspace_allocator kvs_alloc;

static inline void __stack_init(struct free_stack *stack)
{
	stack->top_index = -1;
}

static inline void __stack_push(struct free_stack *stack, struct k_vspace *free_node)
{
	stack->top_index++;
	stack->free_nodes[stack->top_index] = free_node;
}

static inline bool __stack_is_empty(struct free_stack *stack)
{
	return stack->top_index == -1 ? true : false;
}

static inline struct k_vspace *__stack_pop(struct free_stack *stack)
{
	if (__stack_is_empty(stack))
		return NULL;
	return stack->free_nodes[stack->top_index--];
}

static inline void __kvs_init(struct k_vspace *kvs)
{
    uint32_t *pde;

    pde = (uint32_t *)K_PDE_START;
    while (*pde)
        pde++;
    kvs->addr = addr_from_pde(pde);
    while (!*pde)
        pde++;
    kvs->size = addr_from_pde(pde) - kvs->addr;
    init_list_head(&kvs_alloc.list_head);
    list_add(&kvs->list_head, &kvs_alloc.list_head);
}

static inline void __freenode_stack_init(struct k_vspace *kvs)
{
    __stack_init(&kvs_alloc.free_stack);
    for (size_t i = 1; i < KVS_MAX_NODE + 1; i++) 
        __stack_push(&kvs_alloc.free_stack, &kvs[i]);
}

static inline void __vs_allocator_init(uint32_t mem)
{
    __kvs_init((struct k_vspace *)mem);
    __freenode_stack_init((struct k_vspace *)mem);
}

static inline void __vs_reserve(uint32_t v_addr, size_t size)
{
    uint32_t *pde;
    size_t i;

    pde = (uint32_t *)pde_from_addr(v_addr);
    for (i = 0; i < (size / K_PAGE_SIZE) - 1; i++)
        pde[i] = PG_RESERVED_ENTRY | PG_CONTIGUOUS;
    pde[i] = PG_RESERVED_ENTRY;
}

static inline size_t __vs_size_with_free(uint32_t addr)
{
    uint32_t *pde;
    size_t size;

    size = 0;
    pde = (uint32_t *)pde_from_addr(addr);
    do {
        tlb_flush(addr);
        if (*pde & PG_PRESENT)
            free_pages(*pde & 0xFFC00000, K_PAGE_SIZE);
        size += K_PAGE_SIZE;
        addr += K_PAGE_SIZE;
    } while (*pde++ & PG_CONTIGUOUS);
    return size;
}

static inline void __vs_add_and_merge(uint32_t addr, size_t size)
{
    struct k_vspace *cur;
    struct k_vspace *new;
    
    list_for_each_entry(cur, &kvs_alloc.list_head, list_head) {
        if (addr < cur->addr)
            break;
    }
    new = __stack_pop(&kvs_alloc.free_stack);
    if (!new)
        do_panic("Incorrect usage of vs_free");
    new->addr = addr;
    new->size = size;
    list_add_tail(&new->list_head, &cur->list_head);

    cur = list_prev_entry(new, list_head);
    if (!list_entry_is_head(cur, &kvs_alloc.list_head, list_head) && cur->addr + cur->size == new->addr) {
        new->addr = cur->addr;
        new->size += cur->size;
        list_del(&cur->list_head);
        __stack_push(&kvs_alloc.free_stack, cur);
    }
    cur = list_next_entry(new, list_head);
    if (!list_entry_is_head(cur, &kvs_alloc.list_head, list_head) && new->addr + new->size == cur->addr) {
        new->size += cur->size;
        list_del(&cur->list_head);
        __stack_push(&kvs_alloc.free_stack, cur);
    }
}

void vs_free(void *addr)
{
    size_t size;

    size = __vs_size_with_free((uint32_t)addr);
    __vs_add_and_merge((uint32_t)addr, size);
}

void *vs_alloc(size_t size)
{
    struct k_vspace *cur;
    void *ret;

    ret = NULL;
    list_for_each_entry(cur, &kvs_alloc.list_head, list_head) {
        if (size <= cur->size) {
            cur->size -= size;
            ret = (void *)(cur->addr + cur->size);
            __vs_reserve((uint32_t)ret, size);
            if (!cur->size) {
                list_del(&cur->list_head);
                __stack_push(&kvs_alloc.free_stack, cur);
            }
            break;
        }
    }
    return ret;
}

uint32_t pages_initmap(uint32_t p_addr, size_t size, uint32_t flags)
{
    uint32_t *pde;

    pde = (uint32_t *)K_PDE_START;
    while (*pde)
        pde++;
    p_addr = k_addr_erase_offset(p_addr);
    size = align_kpage(size);
    for (size_t i = 0; i < (size / K_PAGE_SIZE); i++) {
        pde[i] = p_addr | flags;
        p_addr += K_PAGE_SIZE;
    }
    return addr_from_pde(pde);
}

uint32_t vmm_init(void)
{
    uint32_t mem;

    mem = alloc_pages(K_PAGE_SIZE);
    if (!mem)
        do_panic("Not enough memory to initialize the virtual memory manager");
    mem = pages_initmap(mem, K_PAGE_SIZE, PG_GLOBAL | PG_PS | PG_RDWR | PG_PRESENT);
    __vs_allocator_init(mem);
    return mem + KVS_MAX_SIZE + sizeof(struct k_vspace);
}