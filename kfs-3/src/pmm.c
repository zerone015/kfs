#include "pmm.h"
#include "utils.h"
#include "panic.h"
#include "paging.h"
#include "vmm.h"

struct buddy_allocator bd_alloc;

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

static inline uint64_t __ram_size(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uint64_t ram_size;
    
    ram_size = 0;
    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (ram_size < mmap[i].addr + mmap[i].len)
                ram_size = mmap[i].addr + mmap[i].len;
        }
	}
    return ram_size;
}

static inline size_t __bitmap_size(uint64_t ram_size)
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

static inline void __bitmap_init(uint32_t v_addr, uint64_t ram_size)
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
    uint32_t addr;
    size_t page_count;

    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            addr = (uint32_t)mmap[i].addr;
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

static inline uint32_t __bitmap_memory_reserve(uint64_t ram_size, multiboot_memory_map_t* mmap, size_t mmap_count)
{
    multiboot_memory_map_t *useful_mmap;
    size_t bitmap_size;
    uint32_t v_addr;

    bitmap_size = __bitmap_size(ram_size);
    useful_mmap = __bitmap_memory(mmap, mmap_count, bitmap_size);
    if (!useful_mmap)
        panic("Not enough memory for bitmap allocation!");
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

static inline void __page_allocator_init(uint64_t ram_size, multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uint32_t v_addr;

    v_addr = __bitmap_memory_reserve(ram_size, mmap, mmap_count);
    __bitmap_init(v_addr, ram_size);
    __pages_register(mmap, mmap_count);
}

static inline uint32_t __mmap_pages_map(uint32_t mmap_addr, size_t mmap_size)
{
    uint32_t v_addr;

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
    uint32_t *kpage_dir;

    kpage_dir = (uint32_t *)dir_from_addr(mbd->mmap_addr);
    for (size_t i = 0; i < ((align_kpage(mbd->mmap_length) + K_PAGE_SIZE) / K_PAGE_SIZE); i++) {
        kpage_dir[i] = 0;
        tlb_flush(mbd->mmap_addr);
        mbd->mmap_addr += K_PAGE_SIZE;
    }
    kpage_dir = (uint32_t *)dir_from_addr(mbd);
    *kpage_dir = 0;
    tlb_flush((uint32_t)mbd);
}

uint32_t alloc_pages(size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t bit_offset;

	if (size > MAX_BLOCK_SIZE)
		return 0;
	order = 0;
	while (size > __block_size(order))
		order++;
	for (size_t i = order; i < MAX_ORDER; i++) {
		if (bd_alloc.orders[i].free_count) {
			bitmap = bd_alloc.orders[i].bitmap;
			while (!*bitmap)
				bitmap++;
			bit_offset = __bit_first_set_offset(bd_alloc.orders[i].bitmap, bitmap);
			__bit_unset(bitmap, __builtin_clz(*bitmap));
			bd_alloc.orders[i].free_count--;
			while (i > order) {
				i--;
				bit_offset <<= 1;
				bitmap = bd_alloc.orders[i].bitmap;
				__bit_set(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31);
				bd_alloc.orders[i].free_count++;
			}
			return bit_offset << (PAGE_SHIFT + order);
		}
	}
	return 0;
}

void free_pages(uint32_t addr, size_t size)
{
	uint32_t *bitmap;
	size_t order;
	size_t bit_offset;

	if (!addr || size > MAX_BLOCK_SIZE)
		return;
	order = 0;
	while (size > __block_size(order))
		order++;
	bit_offset = addr >> (PAGE_SHIFT + order);
	bitmap = bd_alloc.orders[order].bitmap;
	while (order < MAX_ORDER - 1) {
		if (!__bit_check(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31))
			break;
		__bit_unset(&bitmap[bit_offset >> 5], (bit_offset ^ 1) & 31);
		bd_alloc.orders[order].free_count--;
		bit_offset >>= 1;
		order++;
		bitmap = bd_alloc.orders[order].bitmap;
	}
	__bit_set(&bitmap[bit_offset >> 5], bit_offset & 31);
	bd_alloc.orders[order].free_count++;
}

void pmm_init(multiboot_info_t* mbd)
{
    uint64_t ram_size;
    multiboot_memory_map_t mmap[MAX_MMAP];
    size_t mmap_count;

    mbd->mmap_addr = __mmap_pages_map(mbd->mmap_addr, mbd->mmap_length);
    mmap_count = __mmap_count(mbd->mmap_length);
    if (mmap_count > MAX_MMAP)
        panic("The GRUB memory map is too large!");
    __mmap_memcpy(mmap, (multiboot_memory_map_t *)mbd->mmap_addr, mmap_count);
    __mbd_mmap_pages_unmap(mbd);
    __mmap_sanitize(mmap, mmap_count);
    ram_size = __ram_size(mmap, mmap_count);
    __kernel_memory_reserve(mmap, mmap_count);
    __memory_align(mmap, mmap_count);
    __page_allocator_init(ram_size, mmap, mmap_count);
}