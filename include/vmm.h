#ifndef _VMM_H
#define _VMM_H

#include "vmm_defs.h"
#include "list.h"
#include "rbtree.h"
#include "pmm.h"
#include "hmm.h"
#include "daemon.h"
#include <stdint.h>
#include <stdbool.h>

uintptr_t vmm_init(void);
uintptr_t pages_initmap(uintptr_t p_addr, size_t size, int flags);
size_t vb_size(void *addr);
void *vb_alloc(size_t size);
void vb_free(void *addr);

static inline void vblock_bybase_add(struct user_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->base < rb_entry(parent, struct user_vblock, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_base, parent, cur);
    rb_insert_color(&new->by_base, root);
}

static inline void vblock_bysize_add(struct user_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->size < rb_entry(parent, struct user_vblock, by_size)->size)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_size, parent, cur);
    rb_insert_color(&new->by_size, root);
}

static inline void mapping_file_add(struct mapping_file *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->base < rb_entry(parent, struct mapping_file, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_base, parent, cur);
    rb_insert_color(&new->by_base, root);
}

static inline void vblocks_cleanup(struct user_vblock_tree *vblocks)
{
    struct rb_root *root;
    struct user_vblock *cur, *tmp;

    root = &vblocks->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        kfree(cur);
    }
    (&vblocks->by_base)->rb_node = NULL;
    (&vblocks->by_size)->rb_node = NULL;
}

static inline __attribute__((always_inline)) void mapping_files_cleanup(struct mapping_file_tree *mapping_files, bool do_mapping_free, bool do_tlb_invl)
{
    struct mapping_file *cur, *tmp;
    struct rb_root *root;
    uint32_t *pte;
    page_t page;

    root = &mapping_files->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        if (do_mapping_free) {
            pte = pte_from_addr(cur->base);
            for (size_t i = 0; i < (cur->size / PAGE_SIZE); i++) {
                page = page_from_pte(pte[i]);
                if (is_cow(pte[i])) {
                    if (!page_is_shared(page)) {
                        free_pages(page, PAGE_SIZE);
                        page_ref_clear(page);
                    }
                    else {
                        page_ref_dec(page);
                    }
                    if (do_tlb_invl)
                        tlb_invl(cur->base + (i*PAGE_SIZE));
                } 
                else if (page_is_present(pte[i])) {
                    free_pages(page, PAGE_SIZE);
                    if (do_tlb_invl)
                        tlb_invl(cur->base + (i*PAGE_SIZE));
                }
            }
        }
        kfree(cur);
    }
    root->rb_node = NULL;
}

static inline __attribute__((always_inline)) void pgdir_cleanup(bool do_recycle)
{
    uint32_t *pgdir, *pgtab;

    pgdir = current_pgdir();
    for (size_t i = 0; i < 768; i++) {
        if (pgdir[i]) {
            free_pages(page_4kb_from_pde(pgdir[i]), PAGE_SIZE);
            if (do_recycle) {
                pgdir[i] = 0;
                pgtab = pgtab_from_pdi(i);
                tlb_invl((uintptr_t)pgtab);
            }
        }
    }
}

static inline __attribute__((always_inline)) void user_vspace_cleanup
    (struct user_vblock_tree *vblocks, struct mapping_file_tree *mapping_files, int flags)
{
    vblocks_cleanup(vblocks);
    mapping_files_cleanup(mapping_files, flags & CL_MAPPING_FREE, flags & CL_TLB_INVL);
    pgdir_cleanup(flags & CL_RECYCLE);
}

static inline void *tmp_vmap(page_t page)
{
    uint32_t *pte;

    pte = pgtab_from_pdi(1022);
    while (*pte)
        pte++;
    *pte = page | PG_RDWR | PG_PRESENT;
    return (void *)addr_from_pte(pte);
}

static inline void tmp_vunmap(void *mem)
{
    uint32_t *pte;

    pte = pte_from_addr(mem);
    *pte = 0;
    tlb_invl((uintptr_t)mem);
}

static inline void cow_handle(uint32_t *pte, uintptr_t addr) 
{
    page_t page, new_page;
    void *new;

    page = page_from_pte(*pte);
    if (page_is_shared(page)) {
        new_page = alloc_pages(PAGE_SIZE);
        new = tmp_vmap(new_page);
        memcpy32(new, (void *)addr_erase_offset(addr), PAGE_SIZE / 4);
        tmp_vunmap(new);
        *pte = new_page | (*pte & 0x1FF) | PG_RDWR;
        page_ref_dec(page);
    } else {
        *pte = (*pte & ~PG_COW_RDWR) | PG_RDWR;
        page_ref_clear(page);
    }
    tlb_invl(addr);
}

#endif