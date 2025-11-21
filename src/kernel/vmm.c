#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"
#include "kmalloc.h"

static struct list_head free_list;              /* free kernel vblocks */
static struct k_vblock objs[K_VBLOCK_MAX];
static struct k_vblock_pool obj_pool;           /* free k_vblock objects */

static bool obj_pool_empty(void)
{
	return obj_pool.top == -1 ? true : false;
}

static void obj_pool_push(struct k_vblock *obj)
{
	obj_pool.top++;
	obj_pool.objs[obj_pool.top] = obj;
}

static struct k_vblock *obj_pool_pop(void)
{
	if (obj_pool_empty())
		return NULL;
	return obj_pool.objs[obj_pool.top--];
}

static void vb_init(void)
{
    uint32_t *pde;

    pde = pgdir_kernel_base();
    while (*pde)
        pde++;

    objs[0].base = va_from_pde(pde);

    while (!*pde)
        pde++;
    objs[0].size = va_from_pde(pde) - objs[0].base;

    init_list_head(&free_list);
    list_add(&objs[0].list_head, &free_list);
}

static void vb_pool_init(void)
{
	obj_pool.top = -1;
    for (size_t i = 1; i < K_VBLOCK_MAX; i++) 
        obj_pool_push(&objs[i]);
}

static void vb_reserve(uintptr_t v_addr, size_t size)
{
    uint32_t *pde;
    uint32_t flags;
    size_t i;

    pde = pde_from_va(v_addr);
    flags = PG_RESERVED | PG_GLOBAL | PG_PS | PG_RDWR;

    for (i = 0; i < (size / PAGE_LARGE_SIZE) - 1; i++)
        pde[i] = flags | PG_CONTIGUOUS;
    pde[i] = flags;
}

static size_t vb_size_with_free(uintptr_t addr)
{
    uint32_t *pde;
    size_t size;
    size_t pa;

    size = 0;
    pde = pde_from_va(addr);
    do {
        if (page_is_present(*pde)) {
            pa = pa_from_pde(*pde, PAGE_LARGE_SIZE);
            free_pages(pa, PAGE_LARGE_SIZE);
            tlb_invl(addr);
        }
        size += PAGE_LARGE_SIZE;
        addr += PAGE_LARGE_SIZE;
    } while (*pde++ & PG_CONTIGUOUS);
    return size;
}

static void vb_add_and_merge(uintptr_t addr, size_t size)
{
    struct k_vblock *cur;
    struct k_vblock *new;
    
    list_for_each_entry(cur, &free_list, list_head) {
        if (addr < cur->base)
            break;
    }

    new = obj_pool_pop();
    new->base = addr;
    new->size = size;

    list_add_tail(&new->list_head, &cur->list_head);

    cur = list_prev_entry(new, list_head);

    if (!list_entry_is_head(cur, &free_list, list_head) 
        && cur->base + cur->size == new->base) {
        new->base = cur->base;
        new->size += cur->size;
        list_del(&cur->list_head);
        obj_pool_push(cur);
    }

    cur = list_next_entry(new, list_head);

    if (!list_entry_is_head(cur, &free_list, list_head) 
        && new->base + new->size == cur->base) {
        new->size += cur->size;
        list_del(&cur->list_head);
        obj_pool_push(cur);
    }
}

size_t vb_size(void *addr)
{
    uint32_t *pde;
    size_t size;

    size = 0;
    pde = pde_from_va(addr);
    do {
        size += PAGE_LARGE_SIZE;
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
    struct k_vblock *cur;
    void *mem;

    mem = NULL;
    list_for_each_entry(cur, &free_list, list_head) {
        if (size <= cur->size) {
            cur->size -= size;
            mem = (void *)(cur->base + cur->size);
            vb_reserve((uintptr_t)mem, size);
            if (!cur->size) {
                list_del(&cur->list_head);
                obj_pool_push(cur);
            }
            break;
        }
    }
    return mem;
}

/* Map `count` consecutive 4MB pages before the VMM allocator is available. */
uintptr_t early_map(size_t pa_base, size_t count, int flags)
{
    uint32_t *pde;

    pde = pgdir_kernel_base();
    while (*pde)
        pde++;

    for (size_t i = 0; i < count; i++) {
        pde[i] = pa_base | flags;
        pa_base += PAGE_LARGE_SIZE;
    }

    return va_from_pde(pde);
}

/*
* Kernel virtual address space initialization.
* User virtual address spaces are not initialized at boot;
* they are defined in task_struct and initialized during fork().
*/
void vmm_init(void)
{
    vb_init();
    vb_pool_init();
}