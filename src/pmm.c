#include "pmm.h"
#include "utils.h"
#include "panic.h"
#include "paging.h"
#include "vmm.h"
#include "hmm.h"
#include <stdbool.h>

static struct buddy_allocator bd_alloc;
uint16_t *page_ref;
uint64_t ram_size;
struct page *page_map;

/*
 * Trims memory regions that exceed the 4 GB physical address limit.
 * Since PAE (Physical Address Extension) is not supported at the moment,
 * the kernel cannot access physical addresses above 4 GB.
 * Any region starting beyond this limit is marked as reserved,
 * and regions that partially cross the boundary are truncated to fit within it.
 */
static void mmap_trim_highmem(struct memory_map *mmap)
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
 * aligned to PAGE_SIZE, and stores it in the global 'ram_size'.
 *
 * The resulting RAM size is used to determine the size of physical memory
 * metadata structures (e.g., the struct page array).
 */
static void ram_size_init(struct memory_map *mmap)
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
    ram_size = align_up(max_end, PAGE_SIZE);
}

/*
 * Aligns all usable memory regions in the memory map to page boundaries.
 * BIOS-provided memory map regions are not guaranteed to be page-aligned,
 * so this adjustment ensures that the physical memory allocator can manage
 * pages cleanly without overlapping partial regions.
 */
static void mmap_align(struct memory_map *mmap)
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

static void pages_register(struct memory_map *mmap)
{
    multiboot_memory_map_t *e;
    uintptr_t addr;
    uintptr_t end;
    uintptr_t usable_end;
    size_t bsize;
    size_t order;

    for (size_t i = 0; i < mmap->count; i++) {
        e = &mmap->regions[i];

        if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        addr = (uintptr_t)e->addr;
        end = addr + e->len;
        usable_end = align_down(end, PAGE_SIZE);

        if (usable_end <= addr)
            continue;

        while (addr < usable_end) {
            for (order = MAX_ORDER - 1; order > 0; order--) {
                bsize = block_size(order);

                if (is_aligned(addr, bsize) &&
                    addr + bsize <= usable_end)
                    break;
            }
            bsize = block_size(order);
            
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
static void mmap_reserve_kernel(struct memory_map *mmap)
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

static uintptr_t mmap_pages_map(uintptr_t mmap_addr, size_t mmap_size)
{
    uintptr_t v_addr;

    mmap_size += K_PAGE_SIZE;
    v_addr = pages_initmap(mmap_addr, mmap_size, 
                           PG_PS | PG_RDWR | PG_PRESENT);

    return v_addr | k_addr_get_offset(mmap_addr);
}

static size_t find_mmap_count(size_t len)
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

static void mbd_mmap_pages_unmap(multiboot_info_t* mbd)
{
    uint32_t *pde;

    pde = pde_from_addr(mbd->mmap_addr);

    for (size_t i = 0; i < ((align_up(mbd->mmap_length, K_PAGE_SIZE) + K_PAGE_SIZE) / K_PAGE_SIZE); i++) {
        pde[i] = 0;
        tlb_invl(mbd->mmap_addr);
        mbd->mmap_addr += K_PAGE_SIZE;
    }
    
    pde = pde_from_addr(mbd);
    *pde = 0;

    tlb_invl((uintptr_t)mbd);
}

/*
 * The memory map provided by GRUB is stored in a region that BIOS marks as usable.
 * In other words, this region lies within memory that the kernel may later overwrite.
 * However, since the memory map is only used briefly within this initialization function,
 * it is much simpler to copy it into a stack array than to mark that region as reserved.
 * Therefore, temporarily map the mmap pages with mmap_pages_map(), copy them,
 * and then unmap the original mapping.
 */
static void mmap_copy_from_grub(struct memory_map *mmap, multiboot_info_t *mbd)
{
    mbd->mmap_addr = mmap_pages_map(mbd->mmap_addr, mbd->mmap_length);

    mmap->count = find_mmap_count(mbd->mmap_length);
    if (mmap->count > MAX_MMAP)
        do_panic("Abnormal number of GRUB memory map regions");
    mmap_memcpy(mmap->regions, (multiboot_memory_map_t *)mbd->mmap_addr, mmap->count);

    mbd_mmap_pages_unmap(mbd);
}

static multiboot_memory_map_t *mmap_largest_available_region(struct memory_map *mmap)
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

        start = align_up(e->addr, K_PAGE_SIZE);
        if (start >= e->addr + e->len)
            continue;

        usable = (e->addr + e->len) - start;
        if (usable > best_len) {
            best_len = usable;
            best = e;
        }
    }
    return best;
}

static void mmap_reserve_metadata(struct memory_map *mmap,
                                                    multiboot_memory_map_t *best,
                                                    size_t metadata_size)
{
    uint64_t orig_addr;
    uint64_t orig_len;
    uint64_t meta_addr;
    uint64_t meta_end;
    uint64_t orig_end;
    size_t count;

    metadata_size = align_up(metadata_size, K_PAGE_SIZE);

    orig_addr = best->addr;
    orig_len = best->len;
    orig_end = orig_addr + orig_len;

    meta_addr = align_up(orig_addr, K_PAGE_SIZE);
    meta_end = meta_addr + metadata_size;
    count = mmap->count;

    /* before */
    if (meta_addr > orig_addr) {
        mmap->regions[count] = *best;
        mmap->regions[count].addr = orig_addr;
        mmap->regions[count].len = meta_addr - orig_addr;
        mmap->regions[count].type = MULTIBOOT_MEMORY_AVAILABLE;
        count++;
    }

    /* metadata */
    best->addr = meta_addr;
    best->len = metadata_size;
    best->type = MULTIBOOT_MEMORY_RESERVED;

    /* after */
    if (meta_end < orig_end) {
        mmap->regions[count] = *best;
        mmap->regions[count].addr = meta_end;
        mmap->regions[count].len = orig_end - meta_end;
        mmap->regions[count].type = MULTIBOOT_MEMORY_AVAILABLE;
        count++;
    }

    mmap->count = count;
}

size_t page_allocator_init(struct memory_map *mmap)
{
    multiboot_memory_map_t *region;
    uintptr_t vaddr;
    size_t total_pages;
    size_t pagemap_size;
    size_t order;
    size_t leftover;

    total_pages = ram_size / PAGE_SIZE;
    pagemap_size = sizeof(struct page) * total_pages;

    region = mmap_largest_available_region(mmap);
    if (!region)
        do_panic("No available RAM for page metadata");

    mmap_reserve_metadata(mmap, region, pagemap_size);

    vaddr = pages_initmap(region->addr, region->len,
                          PG_PS | PG_RDWR | PG_PRESENT);
    memset((void *)vaddr, 0, region->len);

    page_map = (struct page *)vaddr;

    for (order = 0; order < MAX_ORDER; order++)
        init_list_head(&bd_alloc.orders[order].free_list);

    pages_register(mmap);

    leftover = region->len - pagemap_size;
    return leftover;
}

static size_t pfn_from_page(struct page *pg)
{
	return (size_t)(pg - page_map);
}

static struct page *page_from_pfn(size_t pfn)
{
	return page_map + pfn;
}

static size_t page_order(size_t size)
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

/*
 * Buddy allocator invariant:
 * - free_list[order] contains only blocks whose PFN is aligned to
 * 2^order pages (i.e., each block is the left/base of its pair).
 *
 * Free/alloc operations maintain this invariant.
 */
size_t alloc_pages(size_t size)
{
	struct page *page = NULL;   /* GCC false-positive: maybe-uninitialized */
	struct page *right_page;
	struct list_head *free_list;
	size_t order;
	size_t cur_order;
	size_t pfn;
	size_t right_pfn;
	size_t bpages;

	order = page_order(size);

	for (cur_order = order; cur_order < MAX_ORDER; cur_order++) {
		free_list = &bd_alloc.orders[cur_order].free_list;
		if (!list_empty(free_list)) {
			page = list_first_entry(free_list, struct page, free_list);
			list_del(&page->free_list);
			break;
		}
	}

	if (cur_order == MAX_ORDER)
		return PAGE_NONE;

	pfn = pfn_from_page(page);

	/*
	 * Split a larger block into two smaller buddy blocks.
	 *
	 * Since the block taken from free_list[cur_order] is always base-aligned
	 * (maintained by free_pages()), 'pfn' already points to the left child.
	 *
	 * So right child = pfn + (2^cur_order), and left child (pfn) is split again.
	 */
	while (cur_order > order) {
		cur_order--;

		bpages = block_pages(cur_order);

		right_pfn = pfn + bpages;
		right_page = page_from_pfn(right_pfn);
		right_page->ref_count = 0;
		page_set_free(right_page);

		free_list = &bd_alloc.orders[cur_order].free_list;
		list_add(&right_page->free_list, free_list);
	}

    page = page_from_pfn(pfn);
	page_clear_free(page);
	return phys_addr_from_pfn(pfn);
}

void free_pages(size_t addr, size_t size)
{
	struct page *page;
	struct page *buddy;
	size_t order;
	size_t pfn;
	size_t buddy_pfn;
	size_t bpages;

	order = page_order(size);

	pfn = pfn_from_phys_addr(addr);

	/*
	 * Merge with buddy blocks while possible.
	 *
	 * buddy_pfn = pfn XOR (2^order) gives the other half of the pair.
	 * If buddy is free, remove it and merge into a higher-order block.
	 *
	 * After merging, the new block's base PFN is:
	 * pfn &= ~((2 * bpages) - 1)
	 * which ensures the merged block is base-aligned.
	 */
	while (order < MAX_ORDER - 1) {
		bpages = block_pages(order);

		buddy_pfn = pfn ^ bpages;
		buddy = page_from_pfn(buddy_pfn);

		if (!page_is_free(buddy))
			break;

		list_del(&buddy->free_list);
		page_clear_free(buddy);

        pfn = align_down(pfn, 2*bpages);
		order++;
	}

	page = page_from_pfn(pfn);
	page->ref_count = 0;
	page_set_free(page);

	list_add(&page->free_list, &bd_alloc.orders[order].free_list);
}

void pmm_init(multiboot_info_t *mbd)
{
    struct memory_map mmap;

    mmap_copy_from_grub(&mmap, mbd);
    mmap_trim_highmem(&mmap);
    mmap_reserve_kernel(&mmap);
    mmap_align(&mmap);

    ram_size_init(&mmap);

    page_allocator_init(&mmap);
}

void page_ref_init(void)
{
    size_t page_ref_size;

    page_ref_size = ram_size / PAGE_SIZE * sizeof(uint16_t);
    page_ref = kmalloc(page_ref_size);
    if (!page_ref)
        do_panic("page ref init failed");
    memset(page_ref, 0, page_ref_size);
}
