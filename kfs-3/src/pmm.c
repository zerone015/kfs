#include "pmm.h"
#include "utils.h"
#include "panic.h"
#include "paging.h"
#include "vmm.h"

struct buddy_allocator buddy_allocator;

static void mmap_sanitize(multiboot_memory_map_t* mmap, size_t mmap_count)
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

static uint64_t find_ram_size(multiboot_memory_map_t* mmap, size_t mmap_count)
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

static size_t find_bitmap_size(uint64_t ram_size)
{
    size_t first_size;
    size_t sum;

    sum = 0;
    first_size = bitmap_first_size(ram_size);
    for (size_t order = 0; order < MAX_ORDER; order++) {
        sum += align_4byte(first_size);
        first_size /= 2;
        if (first_size < 32)
            first_size = 32;
    }
    return align_page(sum);
}

static void memory_align(multiboot_memory_map_t* mmap, size_t mmap_count)
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

static void bitmap_init(uint32_t v_addr, uint64_t ram_size)
{
    size_t first_size;

    first_size = bitmap_first_size(ram_size);
    for (size_t order = 0; order < MAX_ORDER; order++) {
        buddy_allocator.orders[order].bitmap = (uint32_t *)v_addr;
        v_addr += align_4byte(first_size);
        first_size /= 2;
        if (first_size < 32)
            first_size = 32;
    }
}

static void frame_register(multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uint32_t addr;
    size_t page_count;

    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            addr = (uint32_t)mmap[i].addr;
            page_count = mmap[i].len / PAGE_SIZE;
            while (page_count--) {
                frame_free(addr, PAGE_SIZE);
                addr += PAGE_SIZE;
            }
        }
	}
}

static void kernel_memory_reserve(multiboot_memory_map_t* mmap, size_t mmap_count)
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

static multiboot_memory_map_t *find_bitmap_memory(multiboot_memory_map_t* mmap, size_t mmap_count, size_t bitmap_size)
{
    for (size_t i = 0; i < mmap_count; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (bitmap_size <= mmap[i].len) 
                return &mmap[i];       
        }
	}
    return NULL;
}

static uint32_t bitmap_memory_reserve(uint64_t ram_size, multiboot_memory_map_t* mmap, size_t mmap_count)
{
    multiboot_memory_map_t *useful_mmap;
    size_t bitmap_size;
    uint32_t v_addr;

    bitmap_size = find_bitmap_size(ram_size);
    useful_mmap = find_bitmap_memory(mmap, mmap_count, bitmap_size);
    if (!useful_mmap)
        panic("Not enough memory for bitmap allocation!");
    v_addr = page_map((uint32_t)useful_mmap->addr, bitmap_size, PAGE_TAB_GLOBAL | PAGE_TAB_RDWR);
    memset((void *)v_addr, 0, bitmap_size);
    if (useful_mmap->len == bitmap_size) {
        useful_mmap->type = MULTIBOOT_MEMORY_RESERVED;
    } else {
        useful_mmap->len -= bitmap_size;
        useful_mmap->addr += bitmap_size;
    }
    return v_addr;
}

static void buddy_allocator_init(uint64_t ram_size, multiboot_memory_map_t* mmap, size_t mmap_count)
{
    uint32_t v_addr;

    v_addr = bitmap_memory_reserve(ram_size, mmap, mmap_count);
    bitmap_init(v_addr, ram_size);
    frame_register(mmap, mmap_count);
}

static uint32_t mmap_page_map(uint32_t mmap_addr, size_t mmap_size)
{
    uint32_t v_addr;

    mmap_size = align_page(mmap_size) + PAGE_SIZE;
    v_addr = page_map(addr_erase_offset(mmap_addr), mmap_size, PAGE_TAB_RDWR);
    return v_addr | addr_get_offset(mmap_addr);
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

static void mbd_page_unmap(multiboot_info_t* mbd)
{
    uint32_t *k_page_table;

    k_page_table = (uint32_t *)tab_from_addr(mbd->mmap_addr);
    for (size_t i = 0; i < (align_page(mbd->mmap_length) + PAGE_SIZE) / PAGE_SIZE; i++)
        k_page_table[i] = 0;
    k_page_table = (uint32_t *)tab_from_addr(mbd);
    *k_page_table = 0;
    reload_cr3();
}

void pmm_init(multiboot_info_t* mbd)
{
    uint64_t ram_size;
    multiboot_memory_map_t mmap[MAX_MMAP];
    size_t mmap_count;

    mbd->mmap_addr = mmap_page_map(mbd->mmap_addr, mbd->mmap_length);
    mmap_count = find_mmap_count(mbd->mmap_length);
    if (mmap_count > MAX_MMAP)
        panic("The GRUB memory map is too large!");
    mmap_memcpy(mmap, (multiboot_memory_map_t *)mbd->mmap_addr, mmap_count);
    mbd_page_unmap(mbd);
    mmap_sanitize(mmap, mmap_count);
    ram_size = find_ram_size(mmap, mmap_count);
    kernel_memory_reserve(mmap, mmap_count);
    memory_align(mmap, mmap_count);
    buddy_allocator_init(ram_size, mmap, mmap_count);
}