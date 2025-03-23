#include "vmm.h"
#include "rbtree.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

struct k_vspace_manager kvm;

uint32_t page_map(uint32_t p_addr, size_t size, uint32_t flags)
{
    uint32_t *page_dir;

    page_dir = (uint32_t *)K_PAGE_DIR_BEGIN;
    while (*page_dir)
        page_dir++;
    p_addr = k_addr_erase_offset(p_addr);
    size = align_kpage(size);
    for (size_t i = 0; i < (size / K_PAGE_SIZE); i++) {
        page_dir[i] = p_addr | flags;
        p_addr += K_PAGE_SIZE;
    }
    return addr_from_dir(page_dir);
}

static void kvs_init(void)
{
    struct rb_node *parent, *new;

    kvm.kvs->addr = K_VSPACE_START;
    kvm.kvs->size = K_VLOAD_START - K_VSPACE_START;
    kvm.kvs[1].addr = (uint32_t)kvm.kvs + PAGE_SIZE;
    kvm.kvs[1].size = K_VSPACE_END - kvm.kvs[1].addr + 1;

    new = &kvm.kvs->by_addr;
    rb_link_node(new, NULL, &kvm.rb_aroot.rb_node);
    rb_insert_color(new, &kvm.rb_aroot);

    parent = new;
    new = &kvm.kvs[1].by_addr;
    rb_link_node(new, parent, &parent->rb_right);
    rb_insert_color(new, &kvm.rb_aroot);

    new = &kvm.kvs->by_size;
    rb_link_node(new, NULL, &kvm.rb_sroot.rb_node);
    rb_insert_color(new, &kvm.rb_sroot);

    parent = new;
    new = &kvm.kvs[1].by_size;
    rb_link_node(new, parent, &parent->rb_right);
    rb_insert_color(new, &kvm.rb_sroot);
}

void vmm_init(void)
{
    uint32_t frame;

    frame = frame_alloc(PAGE_SIZE);
    if (!frame)
        panic("Not enough memory to initialize the virtual memory manager");
    // kvm.kvs = (struct k_vspace *)page_map(frame, PAGE_SIZE, PG_GLOBAL | PG_RDWR); 바꿔야함
    kvs_init();
}