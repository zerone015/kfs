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

/*
 * Trims memory regions that exceed the 4 GB physical address limit.
 * Since PAE (Physical Address Extension) is not supported at the moment,
 * the kernel cannot access physical addresses above 4 GB.
 * Any region starting beyond this limit is marked as reserved,
 * and regions that partially cross the boundary are truncated to fit within it.
 */
static void mmap_trim_highmem(struct memory_map *mmap)
{
    multiboot_memory_map_t *entries;

    entries = mmap->entries;
    for (size_t i = 0; i < mmap->count; i++) {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entries[i].addr >= MAX_RAM_SIZE)
                entries[i].type = MULTIBOOT_MEMORY_RESERVED;
            else if (entries[i].addr + entries[i].len > MAX_RAM_SIZE)
                entries[i].len -= entries[i].addr + entries[i].len - MAX_RAM_SIZE;
        }
	}
}

/*
 * Calculates the total physical memory size from the Multiboot memory map.
 * The result indicates the highest physical address available in the system,
 * aligned to a page boundary.
 * This value is used as a reference when determining the size of
 * physical memory management metadata structures (e.g., struct page array).
 */
static uint64_t calc_ram_size(struct memory_map *mmap)
{
    multiboot_memory_map_t *entries;

    entries = mmap->entries;
    for (size_t i = 0; i < mmap->count; i++) {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (ram_size < entries[i].addr + entries[i].len)
                ram_size = entries[i].addr + entries[i].len;
        }
	}
    return align_up(ram_size, PAGE_SIZE);
}

static size_t find_bitmap_size(void)
{
    size_t first_size;
    size_t sum;

    sum = 0;
    first_size = bitmap_first_size(ram_size);
    for (size_t order = 0; order < MAX_ORDER; order++) {
        sum += align_up(first_size, 4);
        first_size /= 2;
        if (first_size < 32)
            first_size = 32;
    }
    return align_up(sum, PAGE_SIZE);
}

/*
 * Aligns all usable memory regions in the memory map to page boundaries.
 * BIOS-provided memory map entries are not guaranteed to be page-aligned,
 * so this adjustment ensures that the physical memory allocator can manage
 * pages cleanly without overlapping partial regions.
 */
static void mmap_align(struct memory_map *mmap)
{
    multiboot_memory_map_t *entries;
    uint64_t aligned_addr;

    entries = mmap->entries;
    for (size_t i = 0; i < mmap->count; i++) {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            aligned_addr = align_up(entries[i].addr, PAGE_SIZE);
            if (entries[i].len <= aligned_addr - entries[i].addr) {
                entries[i].type = MULTIBOOT_MEMORY_RESERVED;
            } else {
                entries[i].len -= aligned_addr - entries[i].addr;
                entries[i].addr = aligned_addr;
            }
        }
	}
}

static void bd_allocator_init(uintptr_t v_addr)
{
    size_t first_size;

    first_size = bitmap_first_size(ram_size);
    for (size_t order = 0; order < MAX_ORDER; order++) {
        bd_alloc.orders[order].bitmap = (uint32_t *)v_addr;
        v_addr += align_up(first_size, 4);
        first_size /= 2;
        if (first_size < 32)
            first_size = 32;
    }
}

static void pages_register(struct memory_map *mmap)
{
    multiboot_memory_map_t *entries;
    uintptr_t addr;
    size_t page_count;

    entries = mmap->entries;
    for (size_t i = 0; i < mmap->count; i++) {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            addr = (uintptr_t)entries[i].addr;
            page_count = entries[i].len / PAGE_SIZE;
            while (page_count--) {
                free_pages(addr, PAGE_SIZE);
                addr += PAGE_SIZE;
            }
        }
	}
}

/*
 * In the memory map provided by GRUB, the physical region where the kernel is loaded
 * is still marked as available memory. This must be corrected before initializing
 * the physical memory allocator to prevent the kernel image from being overwritten.
 * This function marks the kernel's physical load area as reserved in the memory map.
 */
static void mmap_reserve_kernel(struct memory_map *mmap)
{
    multiboot_memory_map_t *entries;

    entries = mmap->entries;
    for (size_t i = 0; i < mmap->count; i++) {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entries[i].addr == K_PLOAD_START) {
                if (entries[i].len == KERNEL_SIZE) {
                    entries[i].type = MULTIBOOT_MEMORY_RESERVED;
                } else {
                    entries[i].len -= KERNEL_SIZE;
                    entries[i].addr += KERNEL_SIZE;
                }
            }
        }
	}
}

static multiboot_memory_map_t *find_bitmap_memory(struct memory_map *mmap, size_t bitmap_size)
{
    multiboot_memory_map_t *entries;

    entries = mmap->entries;
    for (size_t i = 0; i < mmap->count; i++) {
        if (entries[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entries[i].addr < 0x00100000 && bitmap_size <= entries[i].len) 
                return &entries[i];
        }
	}
    return NULL;
}

static uintptr_t bd_allocator_memory_reserve(struct memory_map *mmap)
{
    multiboot_memory_map_t *entry;
    size_t bitmap_size;
    uintptr_t v_addr;

    bitmap_size = find_bitmap_size();
    entry = find_bitmap_memory(mmap, bitmap_size);
    if (!entry)
        do_panic("Not enough memory for pmm bitmap allocation");
    v_addr = K_VSPACE_START | entry->addr;
    memset((void *)v_addr, 0, bitmap_size);
    if (entry->len == bitmap_size) {
        entry->type = MULTIBOOT_MEMORY_RESERVED;
    } else {
        entry->len -= bitmap_size;
        entry->addr += bitmap_size;
    }
    return v_addr;
}

static void page_allocator_init(struct memory_map *mmap)
{
    uintptr_t v_addr;

    v_addr = bd_allocator_memory_reserve(mmap);
    bd_allocator_init(v_addr);
    pages_register(mmap);
}

static uintptr_t mmap_pages_map(uintptr_t mmap_addr, size_t mmap_size)
{
    uintptr_t v_addr;

    mmap_size += K_PAGE_SIZE;
    v_addr = pages_initmap(mmap_addr, mmap_size, PG_PS | PG_RDWR | PG_PRESENT);
    return v_addr | k_addr_get_offset(mmap_addr);
}

static size_t find_mmap_count(size_t mmap_length)
{
    size_t mmap_count;
    size_t mmap_size;

    mmap_count = 0;
    mmap_size = sizeof(multiboot_memory_map_t);
    for (size_t i = mmap_size; i <= mmap_length; i += mmap_size)
        mmap_count++;
    return mmap_count;
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

static bool buddy_is_freeable(uint32_t *bitmap, size_t offset)
{
    return BIT_CHECK(bitmap[offset / 32], (offset ^ 1) % 32);
}

static void buddy_bit_set(uint32_t *bitmap, size_t offset)
{
    BIT_SET(bitmap[offset / 32], (offset ^ 1) % 32);
}

static void buddy_bit_clear(uint32_t *bitmap, size_t offset)
{
    BIT_CLEAR(bitmap[offset / 32], (offset ^ 1) % 32);
}

static void my_bit_set(uint32_t *bitmap, size_t offset)
{
    BIT_SET(bitmap[offset / 32], offset % 32);
}

static void my_bit_clear(uint32_t *bitmap, size_t offset)
{
    BIT_CLEAR(bitmap[offset / 32], offset % 32);
}

static page_t calc_addr(size_t order, size_t offset)
{
    return block_size(order) * offset;   
}

static size_t first_set_bit_offset(uint32_t *bitmap) 
{
    size_t i;
    size_t bit_offset;

    i = 0;
    while (!bitmap[i])
        i++;
    bit_offset = __builtin_ctz(bitmap[i]);
    return bit_offset + 32*i;
}

page_t alloc_pages(size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t offset;

	order = 0;
	while (size > block_size(order))
		order++;
	for (size_t i = order; i < MAX_ORDER; i++) {
		if (bd_alloc.orders[i].free_count > 0) {
            bitmap = bd_alloc.orders[i].bitmap;
			offset = first_set_bit_offset(bitmap);
            my_bit_clear(bitmap, offset);
			bd_alloc.orders[i].free_count--;
			while (i > order) {
				i--;
				offset *= 2;
				bitmap = bd_alloc.orders[i].bitmap;
                buddy_bit_set(bitmap, offset);
				bd_alloc.orders[i].free_count++;
			}
			return calc_addr(order, offset);
		}
	}
	return PAGE_NONE;
}

void free_pages(page_t page, size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t offset;

	order = 0;
	while (size > block_size(order))
		order++;
	offset = page / block_size(order);
	bitmap = bd_alloc.orders[order].bitmap;
	while (order < MAX_ORDER - 1) {
        if (!buddy_is_freeable(bitmap, offset))
            break;
        buddy_bit_clear(bitmap, offset);
		bd_alloc.orders[order].free_count--;
		offset /= 2;
		order++;
		bitmap = bd_alloc.orders[order].bitmap;
	}
    my_bit_set(bitmap, offset);
	bd_alloc.orders[order].free_count++;
}

/*
 * The memory map provided by GRUB is stored in a region that BIOS marks as usable.
 * In other words, this region lies within memory that the kernel may later overwrite.
 * However, since the memory map is only used briefly within this initialization function,
 * it is much simpler to copy it into a stack array than to mark that region as reserved.
 * Therefore, temporarily map the mmap pages with mmap_pages_map(), copy them,
 * and then unmap the original mapping.
 */
static void mmap_copy_from_grub(struct memory_map *mmap,
                                  multiboot_info_t *mbd)
{
    mbd->mmap_addr = mmap_pages_map(mbd->mmap_addr, mbd->mmap_length);
    mmap->count = find_mmap_count(mbd->mmap_length);
    if (mmap->count > MAX_MMAP)
        do_panic("The GRUB memory map is too large");
    mmap_memcpy(mmap->entries, (multiboot_memory_map_t *)mbd->mmap_addr, mmap->count);
    mbd_mmap_pages_unmap(mbd);
}

void pmm_init(multiboot_info_t* mbd)
{
    struct memory_map mmap;

    mmap_copy_from_grub(&mmap, mbd);
    mmap_trim_highmem(&mmap);
    ram_size = calc_ram_size(&mmap);
    mmap_reserve_kernel(&mmap);
    mmap_align(&mmap);
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
