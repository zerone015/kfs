#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"
#include "hmm.h"

static struct list_head vblocks;
static struct ptr_stack ptr_stack;

static bool stack_is_empty(struct ptr_stack *stack)
{
	return stack->top == -1 ? true : false;
}

static void push_ptr(struct ptr_stack *stack, struct kernel_vblock *node)
{
	stack->top++;
	stack->ptrs[stack->top] = node;
}

static struct kernel_vblock *pop_ptr(struct ptr_stack *stack)
{
	if (stack_is_empty(stack))
		return NULL;
	return stack->ptrs[stack->top--];
}

static void vblocks_init(struct kernel_vblock *vblock)
{
    uint32_t *pde;

    pde = (uint32_t *)K_PDE_START;
    while (*pde)
        pde++;
    vblock->base = addr_from_pde(pde);
    while (!*pde)
        pde++;
    vblock->size = addr_from_pde(pde) - vblock->base;
    init_list_head(&vblocks);
    list_add(&vblock->list_head, &vblocks);
}

static void ptr_stack_init(struct kernel_vblock *vblock)
{
	ptr_stack.top = -1;
    for (size_t i = 1; i < K_VBLOCK_MAX; i++) 
        push_ptr(&ptr_stack, &vblock[i]);
}

static void vb_allocator_init(uintptr_t mem)
{
    vblocks_init((struct kernel_vblock *)mem);
    ptr_stack_init((struct kernel_vblock *)mem);
}

static void vb_reserve(uintptr_t v_addr, size_t size)
{
    uint32_t *pde;
    size_t i;

    pde = pde_from_addr(v_addr);
    for (i = 0; i < (size / K_PAGE_SIZE) - 1; i++)
        pde[i] = PG_RESERVED | PG_PS | PG_RDWR | PG_CONTIGUOUS;
    pde[i] = PG_RESERVED | PG_PS | PG_RDWR;
}

static size_t vb_size_with_free(uintptr_t addr)
{
    uint32_t *pde;
    size_t size;

    size = 0;
    pde = pde_from_addr(addr);
    do {
        if (page_is_present(*pde)) {
            tlb_invl(addr);
            free_pages(page_4mb_from_pde(*pde), K_PAGE_SIZE);
        }
        size += K_PAGE_SIZE;
        addr += K_PAGE_SIZE;
    } while (*pde++ & PG_CONTIGUOUS);
    return size;
}

static void vb_add_and_merge(uintptr_t addr, size_t size)
{
    struct kernel_vblock *cur;
    struct kernel_vblock *new;
    
    list_for_each_entry(cur, &vblocks, list_head) {
        if (addr < cur->base)
            break;
    }
    new = pop_ptr(&ptr_stack);
    new->base = addr;
    new->size = size;
    list_add_tail(&new->list_head, &cur->list_head);

    cur = list_prev_entry(new, list_head);
    if (!list_entry_is_head(cur, &vblocks, list_head) && cur->base + cur->size == new->base) {
        new->base = cur->base;
        new->size += cur->size;
        list_del(&cur->list_head);
        push_ptr(&ptr_stack, cur);
    }
    cur = list_next_entry(new, list_head);
    if (!list_entry_is_head(cur, &vblocks, list_head) && new->base + new->size == cur->base) {
        new->size += cur->size;
        list_del(&cur->list_head);
        push_ptr(&ptr_stack, cur);
    }
}

size_t vb_size(void *addr)
{
    uint32_t *pde;
    size_t size;

    size = 0;
    pde = pde_from_addr(addr);
    do {
        size += K_PAGE_SIZE;
    } while (*pde++ & PG_CONTIGUOUS);
    return size;
}

void vb_free(void *addr)
{
    size_t size;

    size = vb_size_with_free((uintptr_t)addr);
    vb_add_and_merge((uintptr_t)addr, size);
}

void *vb_alloc(size_t size)
{
    struct kernel_vblock *cur;
    void *mem;

    mem = NULL;
    list_for_each_entry(cur, &vblocks, list_head) {
        if (size <= cur->size) {
            cur->size -= size;
            mem = (void *)(cur->base + cur->size);
            vb_reserve((uintptr_t)mem, size);
            if (!cur->size) {
                list_del(&cur->list_head);
                push_ptr(&ptr_stack, cur);
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
    size = align_4mb_page(size);
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
    if (mem == PAGE_NONE)
        do_panic("Not enough memory to initialize the virtual memory manager");
    mem = pages_initmap(mem, K_PAGE_SIZE, PG_GLOBAL | PG_PS | PG_RDWR | PG_PRESENT);
    vb_allocator_init(mem);
    return mem + K_VBLOCK_MAX_SIZE;
}