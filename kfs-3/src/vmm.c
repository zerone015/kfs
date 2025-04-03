#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

static struct k_vblock_allocator kvb_alloc;

static inline void __stack_init(struct free_stack *stack)
{
	stack->top_index = -1;
}

static inline void __stack_push(struct free_stack *stack, struct k_vblock *free_node)
{
	stack->top_index++;
	stack->free_nodes[stack->top_index] = free_node;
}

static inline bool __stack_is_empty(struct free_stack *stack)
{
	return stack->top_index == -1 ? true : false;
}

static inline struct k_vblock *__stack_pop(struct free_stack *stack)
{
	if (__stack_is_empty(stack))
		return NULL;
	return stack->free_nodes[stack->top_index--];
}

static inline void __kvb_init(struct k_vblock *kvs)
{
    uint32_t *pde;

    pde = (uint32_t *)K_PDE_START;
    while (*pde)
        pde++;
    kvs->addr = addr_from_pde(pde);
    while (!*pde)
        pde++;
    kvs->size = addr_from_pde(pde) - kvs->addr;
    init_list_head(&kvb_alloc.list_head);
    list_add(&kvs->list_head, &kvb_alloc.list_head);
}

static inline void __freenode_stack_init(struct k_vblock *kvs)
{
    __stack_init(&kvb_alloc.free_stack);
    for (size_t i = 1; i < KVB_MAX_NODE + 1; i++) 
        __stack_push(&kvb_alloc.free_stack, &kvs[i]);
}

static inline void __vb_allocator_init(uintptr_t mem)
{
    __kvb_init((struct k_vblock *)mem);
    __freenode_stack_init((struct k_vblock *)mem);
}

static inline void __vb_reserve(uintptr_t v_addr, size_t size)
{
    uint32_t *pde;
    size_t i;

    pde = (uint32_t *)pde_from_addr(v_addr);
    for (i = 0; i < (size / K_PAGE_SIZE) - 1; i++)
        pde[i] = PG_RESERVED_ENTRY | PG_CONTIGUOUS;
    pde[i] = PG_RESERVED_ENTRY;
}

static inline size_t __vb_size_with_free(uintptr_t addr)
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

static inline void __vb_add_and_merge(uintptr_t addr, size_t size)
{
    struct k_vblock *cur;
    struct k_vblock *new;
    
    list_for_each_entry(cur, &kvb_alloc.list_head, list_head) {
        if (addr < cur->addr)
            break;
    }
    new = __stack_pop(&kvb_alloc.free_stack);
    new->addr = addr;
    new->size = size;
    list_add_tail(&new->list_head, &cur->list_head);

    cur = list_prev_entry(new, list_head);
    if (!list_entry_is_head(cur, &kvb_alloc.list_head, list_head) && cur->addr + cur->size == new->addr) {
        new->addr = cur->addr;
        new->size += cur->size;
        list_del(&cur->list_head);
        __stack_push(&kvb_alloc.free_stack, cur);
    }
    cur = list_next_entry(new, list_head);
    if (!list_entry_is_head(cur, &kvb_alloc.list_head, list_head) && new->addr + new->size == cur->addr) {
        new->size += cur->size;
        list_del(&cur->list_head);
        __stack_push(&kvb_alloc.free_stack, cur);
    }
}

size_t vb_size(void *addr)
{
    uint32_t *pde;
    size_t size;

    size = 0;
    pde = (uint32_t *)pde_from_addr(addr);
    do {
        size += K_PAGE_SIZE;
    } while (*pde++ & PG_CONTIGUOUS);
    return size;
}

void vb_free(void *addr)
{
    size_t size;

    size = __vb_size_with_free((uintptr_t)addr);
    __vb_add_and_merge((uintptr_t)addr, size);
}

void *vb_alloc(size_t size)
{
    struct k_vblock *cur;
    void *mem;

    mem = NULL;
    list_for_each_entry(cur, &kvb_alloc.list_head, list_head) {
        if (size <= cur->size) {
            cur->size -= size;
            mem = (void *)(cur->addr + cur->size);
            __vb_reserve((uintptr_t)mem, size);
            if (!cur->size) {
                list_del(&cur->list_head);
                __stack_push(&kvb_alloc.free_stack, cur);
            }
            break;
        }
    }
    return mem;
}

uintptr_t pages_initmap(uintptr_t p_addr, size_t size, int flags)
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

uintptr_t vmm_init(void)
{
    uintptr_t mem;

    mem = alloc_pages(K_PAGE_SIZE);
    if (!mem)
        do_panic("Not enough memory to initialize the virtual memory manager");
    mem = pages_initmap(mem, K_PAGE_SIZE, PG_GLOBAL | PG_PS | PG_RDWR | PG_PRESENT);
    __vb_allocator_init(mem);
    return mem + KVB_MAX_SIZE + sizeof(struct k_vblock);
}