#include "pmm.h"
#include "utils.h"
#include "panic.h"
#include "paging.h"
#include "vmm.h"
#include <stdbool.h>

static struct buddy_allocator bd_alloc;
uint64_t ram_size;

static inline void __mmap_sanitize(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmap[i].addr >= MAX_RAM_SIZE)
                mmap[i].type = MULTIBOOT_MEMORY_RESERVED;
            else if (mmap[i].addr + mmap[i].len > MAX_RAM_SIZE)
                mmap[i].len -= mmap[i].addr + mmap[i].len - MAX_RAM_SIZE;
        }
	}
}

static inline void __calc_ram_size(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (ram_size < mmap[i].addr + mmap[i].len)
                ram_size = mmap[i].addr + mmap[i].len;
        }
	}
    ram_size = align_page(ram_size);
}

static inline size_t __bitmap_size(void)
{
    size_t first_size;
    size_t sum;

    sum = 0;
    first_size = __bitmap_first_size(ram_size);
    for (size_t order = 0; order < MAX_ORDER; order++) {
        sum += align_4byte(first_size);
        first_size /= 2;
        if (first_size < 32)
            first_size = 32;
    }
    return align_page(sum);
}

static inline void __memory_align(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uint64_t align_addr;

    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            align_addr = align_page(mmap[i].addr);
            if (mmap[i].len <= align_addr - mmap[i].addr) {
                mmap[i].type = MULTIBOOT_MEMORY_RESERVED;
            } else {
                mmap[i].len -= align_addr - mmap[i].addr;
                mmap[i].addr = align_addr;
            }
        }
	}
}

static inline void __bd_allocator_init(uintptr_t v_addr)
{
    size_t first_size;

    first_size = __bitmap_first_size(ram_size);
    for (size_t order = 0; order < MAX_ORDER; order++) {
        bd_alloc.orders[order].bitmap = (uint32_t *)v_addr;
        v_addr += align_4byte(first_size);
        first_size /= 2;
        if (first_size < 32)
            first_size = 32;
    }
}

static inline void __pages_register(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uintptr_t addr;
    size_t page_count;

    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            addr = (uintptr_t)mmap[i].addr;
            page_count = mmap[i].len / PAGE_SIZE;
            while (page_count--) {
                free_pages(addr, PAGE_SIZE);
                addr += PAGE_SIZE;
            }
        }
	}
}

static inline void __kernel_memory_reserve(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    size_t kernel_size;

    kernel_size = KERNEL_SIZE;
    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmap[i].addr == 0x00100000) {
                if (mmap[i].len == kernel_size) {
                    mmap[i].type = MULTIBOOT_MEMORY_RESERVED;
                } else {
                    mmap[i].len -= kernel_size;
                    mmap[i].addr += kernel_size;
                }
            }
        }
	}
}

static inline multiboot_memory_map_t *__bitmap_memory(multiboot_memory_map_t* mmap, size_t mmap_count, size_t bitmap_size)
{
    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmap[i].addr < 0x00100000 && bitmap_size <= mmap[i].len) 
                return &mmap[i];
        }
	}
    return NULL;
}

static inline uintptr_t __bd_allocator_memory_reserve(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    multiboot_memory_map_t *useful_mmap;
    size_t bitmap_size;
    uintptr_t v_addr;

    bitmap_size = __bitmap_size();
    useful_mmap = __bitmap_memory(mmap, mmap_count, bitmap_size);
    if (!useful_mmap)
        do_panic("Not enough memory for pmm bitmap allocation");
    v_addr = K_VSPACE_START | useful_mmap->addr;
    memset((void *)v_addr, 0, bitmap_size);
    if (useful_mmap->len == bitmap_size) {
        useful_mmap->type = MULTIBOOT_MEMORY_RESERVED;
    } else {
        useful_mmap->len -= bitmap_size;
        useful_mmap->addr += bitmap_size;
    }
    return v_addr;
}

static inline void __page_allocator_init(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uintptr_t v_addr;

    v_addr = __bd_allocator_memory_reserve(mmap, mmap_count);
    __bd_allocator_init(v_addr);
    __pages_register(mmap, mmap_count);
}

static inline uintptr_t __mmap_pages_map(uintptr_t mmap_addr, size_t mmap_size)
{
    uintptr_t v_addr;

    mmap_size += K_PAGE_SIZE;
    v_addr = pages_initmap(mmap_addr, mmap_size, PG_PS | PG_RDWR | PG_PRESENT);
    return v_addr | k_addr_get_offset(mmap_addr);
}

static inline size_t __mmap_count(size_t mmap_length)
{
    size_t mmap_count;
    size_t mmap_size;

    mmap_count = 0;
    mmap_size = sizeof(multiboot_memory_map_t);
    for (size_t i = mmap_size; i <= mmap_length; i += mmap_size)
        mmap_count++;
    return mmap_count;
}

static inline void __mmap_memcpy(multiboot_memory_map_t *dest, multiboot_memory_map_t *src, size_t mmap_count)
{
    for (size_t i = 0; i < mmap_count; i++) 
        memcpy(dest + i, src + i, sizeof(multiboot_memory_map_t));
}

static inline void __mbd_mmap_pages_unmap(multiboot_info_t* mbd)
{
    uint32_t *pde;

    pde = (uint32_t *)pde_from_addr(mbd->mmap_addr);
    for (size_t i = 0; i < ((align_kpage(mbd->mmap_length) + K_PAGE_SIZE) / K_PAGE_SIZE); i++) {
        pde[i] = 0;
        tlb_flush(mbd->mmap_addr);
        mbd->mmap_addr += K_PAGE_SIZE;
    }
    pde = (uint32_t *)pde_from_addr(mbd);
    *pde = 0;
    tlb_flush((uintptr_t)mbd);
}

static inline bool __buddy_is_freeable(uint32_t *bitmap, size_t offset)
{
    return BIT_CHECK(bitmap[offset / 32], (offset ^ 1) % 32);
}

static inline void __buddy_bit_set(uint32_t *bitmap, size_t offset)
{
    BIT_SET(bitmap[offset / 32], (offset ^ 1) % 32);
}

static inline void __buddy_bit_clear(uint32_t *bitmap, size_t offset)
{
    BIT_CLEAR(bitmap[offset / 32], (offset ^ 1) % 32);
}

static inline void __bit_set(uint32_t *bitmap, size_t offset)
{
    BIT_SET(bitmap[offset / 32], offset % 32);
}

static inline void __bit_clear(uint32_t *bitmap, size_t offset)
{
    BIT_CLEAR(bitmap[offset / 32], offset % 32);
}

static inline uintptr_t __calc_pfn(size_t order, size_t offset)
{
    return __block_size(order) * offset;   
}

static inline size_t __first_set_bit_offset(uint32_t *bitmap) 
{
    size_t i;
    size_t bit_offset;

    i = 0;
    while (!bitmap[i])
        i++;
    bit_offset = __builtin_ctz(bitmap[i]);
    return bit_offset + 32*i;
}

uintptr_t alloc_pages(size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t offset;

	order = 0;
	while (size > __block_size(order))
		order++;
	for (size_t i = order; i < MAX_ORDER; i++) {
		if (bd_alloc.orders[i].free_count > 0) {
            bitmap = bd_alloc.orders[i].bitmap;
			offset = __first_set_bit_offset(bitmap);
            __bit_clear(bitmap, offset);
			bd_alloc.orders[i].free_count--;
			while (i > order) {
				i--;
				offset *= 2;
				bitmap = bd_alloc.orders[i].bitmap;
                __buddy_bit_set(bitmap, offset);
				bd_alloc.orders[i].free_count++;
			}
			return __calc_pfn(order, offset);
		}
	}
	return ALLOC_PAGES_FAILED;
}

void free_pages(uintptr_t addr, size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t offset;

	order = 0;
	while (size > __block_size(order))
		order++;
	offset = addr / __block_size(order);
	bitmap = bd_alloc.orders[order].bitmap;
	while (order < MAX_ORDER - 1) {
        if (!__buddy_is_freeable(bitmap, offset))
            break;
        __buddy_bit_clear(bitmap, offset);
		bd_alloc.orders[order].free_count--;
		offset /= 2;
		order++;
		bitmap = bd_alloc.orders[order].bitmap;
	}
    __bit_set(bitmap, offset);
	bd_alloc.orders[order].free_count++;
}

void pmm_init(multiboot_info_t* mbd)
{
    multiboot_memory_map_t mmap[MAX_MMAP];
    size_t mmap_count;

    mbd->mmap_addr = __mmap_pages_map(mbd->mmap_addr, mbd->mmap_length);
    mmap_count = __mmap_count(mbd->mmap_length);
    if (mmap_count > MAX_MMAP)
        do_panic("The GRUB memory map is too large");
    __mmap_memcpy(mmap, (multiboot_memory_map_t *)mbd->mmap_addr, mmap_count);
    __mbd_mmap_pages_unmap(mbd);
    __mmap_sanitize(mmap, mmap_count);
    __calc_ram_size(mmap, mmap_count);
    __kernel_memory_reserve(mmap, mmap_count);
    __memory_align(mmap, mmap_count);
    __page_allocator_init(mmap, mmap_count);
}