#include "pmm.h"
#include "utils.h"
#include "panic.h"
#include "paging.h"
#include "vmm.h"
#include "kmalloc.h"
#include <stdbool.h>

static struct buddy_allocator bd_alloc;
struct page *pgmap;
struct meminfo meminfo;

/*
 * Trims memory regions that exceed the 4 GB physical address limit.
 * Since PAE (Physical Address Extension) is not supported at the moment,
 * the kernel cannot access physical addresses above 4 GB.
 * Any region starting beyond this limit is marked as reserved,
 * and regions that partially cross the boundary are truncated to fit within it.
 */
static void mmap_trim_highmem(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *region;
    uint64_t end;

    for (size_t i = 0; i < mmap->count; i++) {
        region = &mmap->regions[i];

        if (region->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        if (region->addr >= MAX_RAM_SIZE) {
            region->type = MULTIBOOT_MEMORY_RESERVED;
            continue;
        }

        end = region->addr + region->len;
        if (end > MAX_RAM_SIZE)
            region->len -= end - MAX_RAM_SIZE;
    }
}

/*
 * Computes the highest usable physical address from the Multiboot memory map,
 * aligned to PAGE_SIZE, and returns the corresponding page count.
 *
 * The resulting value is used to determine the size of physical memory
 * metadata structures (e.g., the struct page array).
 */
static size_t pgcount_compute(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *region;
    uint64_t max_end;
    uint64_t end;

    max_end = 0;

    for (size_t i = 0; i < mmap->count; i++) {
        region = &mmap->regions[i];

        if (region->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        end = region->addr + region->len;
        if (max_end < end)
            max_end = end;
    }
    return ceil_div(max_end, PAGE_SIZE);
}

static void meminfo_init(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *r;
    uint64_t start, end;
    uint64_t bytes;
    size_t pages;

    for (size_t i = 0; i < mmap->count; i++) {
        r = &mmap->regions[i];

        if (r->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        start = r->addr;
        end = align_down(r->addr + r->len, PAGE_SIZE);

        if (end <= start)
            continue;

        bytes = end - start;
        pages = bytes / PAGE_SIZE;

        meminfo.usable_pages += pages;
    }
}

/*
 * Aligns all usable memory regions in the memory map to page boundaries.
 * BIOS-provided memory map regions are not guaranteed to be page-aligned,
 * so this adjustment ensures that the physical memory allocator can manage
 * pages cleanly without overlapping partial regions.
 */
static void mmap_align(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *region;
    uint64_t aligned_addr;
    uint64_t delta;

    for (size_t i = 0; i < mmap->count; i++) {
        region = &mmap->regions[i];

        if (region->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        aligned_addr = align_up(region->addr, PAGE_SIZE);
        delta = aligned_addr - region->addr;

        if (region->len <= delta) {
            region->type = MULTIBOOT_MEMORY_RESERVED;
        } else {
            region->len -= delta;
            region->addr  = aligned_addr;
        }
    }
}

/* Try to register pages in the largest blocks possible. */
static void pages_register(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *e;
    size_t addr;
    size_t end;
    size_t usable_end;
    size_t bsize;
    size_t order;

    for (size_t i = 0; i < mmap->count; i++) {
        e = &mmap->regions[i];

        if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        addr = (size_t)e->addr;
        end = addr + e->len;
        usable_end = align_down(end, PAGE_SIZE);

        while (addr < usable_end) {
            for (order = MAX_ORDER - 1; order > 0; order--) {
                bsize = block_size(order);

                if (is_aligned(addr, bsize) &&
                    addr + bsize <= usable_end)
                    break;
            }
            if (order == 0)
                bsize = PAGE_SIZE;

            free_pages(addr, bsize);
            addr += bsize;
        }
    }
}

/*
 * In the memory map provided by GRUB, the physical region where the kernel is loaded
 * is still marked as available memory. This must be corrected before initializing
 * the physical memory allocator to prevent the kernel image from being overwritten.
 *
 * The kernel is physically loaded at 1 MiB (K_PLOAD_START = 0x00100000), as defined
 * by the linker script and required by the boot flow. On PC-compatible hardware,
 * the 64 KiB region immediately below 1 MiB (0x000F0000–0x000FFFFF) is always the
 * motherboard BIOS area and is therefore permanently reserved. This architectural
 * constraint guarantees that the next memory region in the E820 map must begin
 * exactly at 1 MiB. In other words, if the system has successfully booted the kernel,
 * then the E820 memory map must include an region with:
 *
 *     region->type == MULTIBOOT_MEMORY_AVAILABLE
 *     region->addr == K_PLOAD_START   // must be exactly 1 MiB
 *
 * GRUB verifies that the kernel’s physical load range lies entirely within such an
 * available memory region before placing the kernel at 1 MiB. If no region begins
 * at this exact address, the system would not have been able to boot in the first place.
 *
 */
static void mmap_reserve_kernel(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *region;

    for (size_t i = 0; i < mmap->count; i++) {
        region = &mmap->regions[i];

        if (region->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        if (region->addr != K_PLOAD_START)
            continue;

        if (region->len == KERNEL_SIZE) {
            region->type = MULTIBOOT_MEMORY_RESERVED;
        } else {
            region->len -= KERNEL_SIZE;
            region->addr += KERNEL_SIZE;
        }
    }
}

static uintptr_t mmap_map(uintptr_t pa, size_t size)
{
    uintptr_t offset;
    uintptr_t pa_base;
    size_t map_span;   // full range to map (offset + size); may cross 4MB boundaries
    size_t count;
    uintptr_t v_base;

    offset = offset_in(pa, PAGE_LARGE_SIZE);
    pa_base = align_down(pa, PAGE_LARGE_SIZE);
    map_span = offset + size;
    count = ceil_div(map_span, PAGE_LARGE_SIZE);

    v_base = early_map(pa_base, count,
                       PG_PS | PG_RDWR | PG_PRESENT);

    return v_base + offset;
}

static size_t mmap_count_from(size_t len)
{
    size_t count;
    size_t size;

    count = 0;
    size = sizeof(multiboot_memory_map_t);

    for (size_t i = size; i <= len; i += size)
        count++;

    return count;
}

static void mmap_memcpy(multiboot_memory_map_t *dest, multiboot_memory_map_t *src, size_t mmap_count)
{
    for (size_t i = 0; i < mmap_count; i++) 
        memcpy(dest + i, src + i, sizeof(multiboot_memory_map_t));
}

static void mmap_unmap(multiboot_info_t *mbd)
{
    size_t offset;
    uintptr_t va_base;
    size_t total;
    size_t count;
    uint32_t *pde;

    /* Unmap the temporary virtual area created by mmap_map() in mmap_copy_from_grub() */
    offset = offset_in(mbd->mmap_addr, PAGE_LARGE_SIZE);
    va_base = align_down(mbd->mmap_addr, PAGE_LARGE_SIZE);
    total = offset + mbd->mmap_length;
    count = ceil_div(total, PAGE_LARGE_SIZE);

    pde = pde_from_va(va_base);
    for (size_t i = 0; i < count; i++) {
        pde[i] = 0;
        tlb_invl(va_base + i*PAGE_LARGE_SIZE);
    }

    /* mapped at a fixed PDE set in boot.asm */
    va_base = align_down(mbd, PAGE_LARGE_SIZE);
    pde = pde_from_va(va_base);
    *pde = 0;
    tlb_invl(va_base);
}

/*
 * The memory map provided by GRUB is stored in a region that BIOS marks as usable.
 * In other words, this region lies within memory that the kernel may later overwrite.
 * However, since the memory map is only used briefly within this initialization function,
 * it is much simpler to copy it into a stack array than to mark that region as reserved.
 * Therefore, temporarily map the mmap pages with mmap_map(), copy them,
 * and then unmap the original mapping.
 */
static void mmap_copy_from_grub(struct mmap_buffer *mmap, multiboot_info_t *mbd)
{
    mbd->mmap_addr = mmap_map(mbd->mmap_addr, mbd->mmap_length);

    mmap->count = mmap_count_from(mbd->mmap_length);
    if (mmap->count > MAX_MMAP)
        do_panic("Abnormal number of GRUB memory map regions");
        
    mmap_memcpy(mmap->regions, (multiboot_memory_map_t *)mbd->mmap_addr, mmap->count);

    mmap_unmap(mbd);
}

static multiboot_memory_map_t *mmap_largest_available_region(struct mmap_buffer *mmap,
                                                             size_t size)
{
    multiboot_memory_map_t *best;
    multiboot_memory_map_t *e;
    uint64_t best_len;
    uint64_t start;
    uint64_t usable;

    best = NULL;
    best_len = 0;

    for (size_t i = 0; i < mmap->count; i++) {
        e = &mmap->regions[i];

        if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        start = align_up(e->addr, PAGE_LARGE_SIZE);
        if (start + size > e->addr + e->len)
            continue;

        usable = (e->addr + e->len) - start;
        if (usable > best_len) {
            best_len = usable;
            best = e;
        }
    }
    return best;
}

static void mmap_reserve_metadata(struct mmap_buffer *mmap,
                                  multiboot_memory_map_t *region,
                                  size_t meta_size)
{
    uint64_t orig_addr;
    uint64_t orig_len;
    uint64_t meta_addr;
    uint64_t meta_end;
    uint64_t orig_end;
    size_t count;

    orig_addr = region->addr;
    orig_len = region->len;
    orig_end = orig_addr + orig_len;

    meta_addr = align_up(orig_addr, PAGE_LARGE_SIZE);
    meta_end = meta_addr + meta_size;
    count = mmap->count;

    /* before */
    if (meta_addr > orig_addr) {
        mmap->regions[count] = *region;
        mmap->regions[count].addr = orig_addr;
        mmap->regions[count].len = meta_addr - orig_addr;
        mmap->regions[count].type = MULTIBOOT_MEMORY_AVAILABLE;
        count++;
    }

    /* metadata */
    region->addr = meta_addr;
    region->len = meta_size;
    region->type = MULTIBOOT_MEMORY_RESERVED;

    /* after */
    if (meta_end < orig_end) {
        mmap->regions[count] = *region;
        mmap->regions[count].addr = meta_end;
        mmap->regions[count].len = orig_end - meta_end;
        mmap->regions[count].type = MULTIBOOT_MEMORY_AVAILABLE;
        count++;
    }
    
    mmap->count = count;
}

static uintptr_t __pmm_init(struct mmap_buffer *mmap)
{
    multiboot_memory_map_t *region;
    size_t pgcount;
    size_t pgmap_size;
    size_t aligned_size;
    size_t order;
    uintptr_t heap_start;

    pgcount = pgcount_compute(mmap);
    pgmap_size = sizeof(struct page) * pgcount;
    
    aligned_size = align_up(pgmap_size, PAGE_LARGE_SIZE);

    region = mmap_largest_available_region(mmap, aligned_size);
    if (!region)
        do_panic("No available RAM for page metadata");

    mmap_reserve_metadata(mmap, region, aligned_size);

    pgmap = (struct page *)early_map(region->addr,
                                     aligned_size / PAGE_LARGE_SIZE,
                                     PG_GLOBAL | PG_PS | PG_RDWR | PG_PRESENT);
    memset((void *)pgmap, 0, aligned_size);

    for (order = 0; order < MAX_ORDER; order++)
        init_list_head(&bd_alloc.orders[order].free_list);

    meminfo_init(mmap);
    
    pages_register(mmap);

    heap_start = (size_t)pgmap + pgmap_size;
    return heap_start;
}

static size_t pfn_from_page(struct page *pg)
{
	return (size_t)(pg - pgmap);
}

static size_t order_compute(size_t size)
{
	size_t order = 0;

	while (size > block_size(order))
		order++;
	return order;
}

static bool page_is_free(struct page *page)
{
	return page->flags & PAGE_FREE;
}

static void page_clear_free(struct page *page)
{
	page->flags &= ~PAGE_FREE;
}

static void page_set_free(struct page *page)
{
	page->flags |= PAGE_FREE;
}

uintptr_t alloc_pages(size_t size)
{
    struct page *page = NULL;   /* GCC false-positive: maybe-uninitialized */
    struct page *right_page;
    struct list_head *free_list;
    size_t order;
    size_t cur_order;
    size_t pfn;
    size_t right_pfn;
    size_t bpages;

    order = order_compute(size);

    for (cur_order = order; cur_order < MAX_ORDER; cur_order++) {
        free_list = &bd_alloc.orders[cur_order].free_list;
        if (!list_empty(free_list)) {
            page = list_first_entry(free_list, struct page, free_list);
            page->ref = 1;
            list_del(&page->free_list);
            page_clear_free(page);
            break;
        }
    }

    if (cur_order == MAX_ORDER)
        return PAGE_NONE;

    pfn = pfn_from_page(page);

    while (cur_order > order) {
        cur_order--;

        bpages = block_pages(cur_order);

        right_pfn = pfn + bpages;
        right_page = page_from_pfn(right_pfn);
        right_page->ref = 0;
        right_page->order = cur_order;
        page_set_free(right_page);

        free_list = &bd_alloc.orders[cur_order].free_list;
        list_add(&right_page->free_list, free_list);
    }
    
    return pa_from_pfn(pfn);
}

void free_pages(size_t addr, size_t size)
{
    struct page *page;
    struct page *buddy;
    size_t order;
    size_t pfn;
    size_t buddy_pfn;
    size_t bpages;

    order = order_compute(size);

    pfn = pfn_from_pa(addr);

    while (order < MAX_ORDER - 1) {
        bpages = block_pages(order);

        buddy_pfn = pfn ^ bpages;
        buddy = page_from_pfn(buddy_pfn);

        if (!(page_is_free(buddy) && buddy->order == order))
            break;

        list_del(&buddy->free_list);
        page_clear_free(buddy);

        pfn = align_down(pfn, 2*bpages);
        order++;
    }

    page = page_from_pfn(pfn);
    page->ref = 0;
    page->order = order;
    page_set_free(page);

    list_add(&page->free_list, &bd_alloc.orders[order].free_list);
}

uintptr_t pmm_init(multiboot_info_t *mbd)
{
    struct mmap_buffer mmap;

    mmap_copy_from_grub(&mmap, mbd);
    mmap_trim_highmem(&mmap);
    mmap_reserve_kernel(&mmap);
    mmap_align(&mmap);
    return __pmm_init(&mmap); 
}