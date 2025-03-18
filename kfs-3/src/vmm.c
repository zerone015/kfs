#include "vmm.h"
#include "rbtree.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

static struct rb_root rb_aroot;
static struct rb_root rb_sroot;
static struct k_vspace *kvs_db;
static size_t kvs_db_rem;

uint32_t page_map(uint32_t p_addr, size_t size, uint32_t mode)
{
    uint32_t *k_page_table;

    k_page_table = (uint32_t *)tab_from_addr(K_VLOAD_END);
    while (*k_page_table)
        k_page_table++;
    for (size_t i = 0; i < size / PAGE_SIZE; i++) {
        k_page_table[i] = p_addr | mode;
        p_addr += PAGE_SIZE; 
    }
    return addr_from_tab(k_page_table);
}

static void kvs_init(void)
{
    struct rb_node *parent, *new;

    kvs_db->addr = K_VSPACE_START;
    kvs_db->size = K_VLOAD_START - K_VSPACE_START;
    kvs_db[1].addr = (uint32_t)kvs_db + PAGE_SIZE;
    kvs_db[1].size = K_VSPACE_END - kvs_db[1].addr + 1;

    new = &kvs_db->by_addr;
    rb_link_node(new, NULL, &rb_aroot.rb_node);
    rb_insert_color(new, &rb_aroot);

    parent = new;
    new = &kvs_db[1].by_addr;
    rb_link_node(new, parent, &parent->rb_right);
    rb_insert_color(new, &rb_aroot);

    new = &kvs_db->by_size;
    rb_link_node(new, NULL, &rb_sroot.rb_node);
    rb_insert_color(new, &rb_sroot);

    parent = new;
    new = &kvs_db[1].by_size;
    rb_link_node(new, parent, &parent->rb_right);
    rb_insert_color(new, &rb_sroot);

    kvs_db_rem -= sizeof(struct k_vspace) * 2;
}

void vmm_init(void)
{
    uint32_t frame;
    struct k_vspace *kvs_db;

    frame = frame_alloc(PAGE_SIZE);
    if (!frame)
        panic("Not enough memory to initialize the virtual memory manager");
    kvs_db = (struct k_vspace *)page_map(frame, PAGE_SIZE, PAGE_TAB_GLOBAL | PAGE_TAB_RDWR);
    kvs_db_rem = PAGE_SIZE;
    kvs_init();
}