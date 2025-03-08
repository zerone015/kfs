#include "pmm.h"
#include "utils.h"
#include "panic.h"
#include "paging.h"

struct buddy_allocator buddy_allocator;

static uint64_t find_ram_size(multiboot_info_t* mbd)
{
	multiboot_memory_map_t *mmmt;
    uint64_t ram_size;
    
    ram_size = 0;
    for (size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		mmmt = (multiboot_memory_map_t *)((mbd->mmap_addr | K_VIRT_ADDR_BEGIN) + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->addr < MAX_RAM_SIZE) {
            if (ram_size < mmmt->addr + mmmt->len)
                ram_size = mmmt->addr + mmmt->len;
        }
	}
    return ram_size;
}

static size_t find_bitmap_size(uint64_t ram_size)
{
    size_t first_size;
    size_t sum;

    first_size = (ram_size + PAGE_SIZE - 1) / PAGE_SIZE / 8;
    sum = 0;
    for (size_t order = 0; order < MAX_ORDER; order++) {
        if (first_size < sizeof(uint32_t))
            first_size = sizeof(uint32_t);
        sum += first_size;
        first_size /= 2;
    }
    return (sum + 0xFFF) & ~0xFFF;
}

static void memory_align(multiboot_info_t* mbd)
{
	multiboot_memory_map_t *mmmt;
    uint64_t align_addr;

    for (size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		mmmt = (multiboot_memory_map_t *)((mbd->mmap_addr | K_VIRT_ADDR_BEGIN) + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->addr < MAX_RAM_SIZE) {
            align_addr = (mmmt->addr + 0xFFF) & ~0xFFF;
            if (mmmt->len <= align_addr - mmmt->addr) {
                mmmt->type = MULTIBOOT_MEMORY_RESERVED;
            } else {
                mmmt->len -= align_addr - mmmt->addr;
                mmmt->addr = align_addr;
            }
        }
	}
}

static uint32_t bitmap_virtual_assign(uint32_t p_addr, size_t bitmap_size)
{
    uint32_t *k_page_table;
    uint32_t page_dir_idx;
    uint32_t page_tab_idx;
    uint32_t v_addr;

    k_page_table = (uint32_t *)K_PAGE_TAB_BEGIN;
    while (*k_page_table)
        k_page_table++;
    page_dir_idx = (((uint32_t)k_page_table) & 0x003FF000) << 10;
    page_tab_idx = ((((uint32_t)k_page_table) & 0x00000FFF) / 4) << 12;
    v_addr = page_dir_idx | page_tab_idx;
    for (size_t page_count = bitmap_size / PAGE_SIZE; page_count; page_count--) {
        *k_page_table = p_addr + 0x003;
        k_page_table++;
        p_addr += PAGE_SIZE; 
    }
    return v_addr;
}

static void bitmap_init(uint32_t v_addr, uint64_t ram_size)
{
    size_t first_size;

    first_size = (ram_size + PAGE_SIZE - 1) / PAGE_SIZE / 8;
    for (size_t order = 0; order < MAX_ORDER; order++) {
        if (first_size < sizeof(uint32_t))
            first_size = sizeof(uint32_t);
        buddy_allocator.orders[order].bitmap = (uint32_t *)v_addr;
        v_addr += first_size;
        first_size /= 2;
    }
}

static void frame_register(multiboot_info_t* mbd)
{
	multiboot_memory_map_t *mmmt;
    uint32_t addr;
    size_t page_count;

    for (size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		mmmt = (multiboot_memory_map_t *)((mbd->mmap_addr | K_VIRT_ADDR_BEGIN) + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->addr < MAX_RAM_SIZE) {
            addr = (uint32_t)mmmt->addr;
            page_count = mmmt->len / PAGE_SIZE;
            while (page_count--) {
                frame_free((void *)addr, PAGE_SIZE);
                addr += PAGE_SIZE; 
            }
        }
	}
}

static void kernel_memory_reserve(multiboot_info_t* mbd)
{
	multiboot_memory_map_t *mmmt;
    size_t kernel_size;

    kernel_size = (size_t)(&_kernel_end) - (size_t)(&_kernel_start);
    for (size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		mmmt = (multiboot_memory_map_t *)((mbd->mmap_addr | K_VIRT_ADDR_BEGIN) + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->addr < MAX_RAM_SIZE) {
            if (mmmt->addr == 0x00100000) {
                if (mmmt->len == kernel_size) {
                    mmmt->type = MULTIBOOT_MEMORY_RESERVED;
                } else {
                    mmmt->len -= kernel_size;
                    mmmt->addr += kernel_size;
                }
            }
        }
	}
}

static multiboot_memory_map_t *find_bitmap_memory(multiboot_info_t* mbd, size_t bitmap_size)
{
    multiboot_memory_map_t *mmmt;

    for (size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
		mmmt = (multiboot_memory_map_t *)((mbd->mmap_addr | K_VIRT_ADDR_BEGIN) + i);
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->addr < MAX_RAM_SIZE) {
            if (bitmap_size <= mmmt->len) 
                return mmmt;       
        }
	}
    return NULL;
}

static uint32_t bitmap_memory_reserve(uint64_t ram_size, multiboot_info_t* mbd)
{
    multiboot_memory_map_t *mmmt;
    size_t bitmap_size;
    uint32_t v_addr;

    bitmap_size = find_bitmap_size(ram_size);
    mmmt = find_bitmap_memory(mbd, bitmap_size);
    if (!mmmt)
        panic("Not enough memory for bitmap allocation!");
    v_addr = bitmap_virtual_assign((uint32_t)mmmt->addr, bitmap_size);
    memset((void *)v_addr, 0, bitmap_size);
    if (mmmt->len == bitmap_size) {
        mmmt->type = MULTIBOOT_MEMORY_RESERVED;
    } else {
        mmmt->len -= bitmap_size;
        mmmt->addr += bitmap_size;
    }
    return v_addr;
}

static void buddy_allocator_init(uint64_t ram_size, multiboot_info_t* mbd)
{
    uint32_t v_addr;

    v_addr = bitmap_memory_reserve(ram_size, mbd);
    bitmap_init(v_addr, ram_size);
    frame_register(mbd);
}

void frame_allocator_init(multiboot_info_t* mbd)
{
    uint64_t ram_size;

    ram_size = find_ram_size(mbd);
    kernel_memory_reserve(mbd);
    memory_align(mbd);
    buddy_allocator_init(ram_size, mbd);
}