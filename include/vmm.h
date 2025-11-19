#ifndef _VMM_H
#define _VMM_H

#include "vmm_defs.h"
#include "list.h"
#include "rbtree.h"
#include "pmm.h"
#include "kmalloc.h"
#include "init.h"
#include <stdint.h>
#include <stdbool.h>

void vmm_init(void);
uintptr_t early_map(size_t pa_base, size_t count, int flags);
size_t vb_size(void *addr);
void *vb_alloc(size_t size);
void vb_free(void *addr);

static inline void vblock_bybase_add(struct u_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->base < rb_entry(parent, struct u_vblock, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_base, parent, cur);
    rb_insert_color(&new->by_base, root);
}

static inline void vblock_bysize_add(struct u_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->size < rb_entry(parent, struct u_vblock, by_size)->size)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_size, parent, cur);
    rb_insert_color(&new->by_size, root);
}

static inline void mapped_vblock_add(struct mapped_vblock *new, struct rb_root *root)
{
    struct rb_node **cur, *parent; 

    cur = &root->rb_node;
    parent = NULL;
    while (*cur) {
        parent = *cur;
        if (new->base < rb_entry(parent, struct mapped_vblock, by_base)->base)
            cur = &parent->rb_left;
        else
            cur = &parent->rb_right;
    }
    rb_link_node(&new->by_base, parent, cur);
    rb_insert_color(&new->by_base, root);
}

static inline void vblocks_cleanup(struct u_vblock_tree *vblocks)
{
    struct rb_root *root;
    struct u_vblock *cur, *tmp;

    root = &vblocks->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        kfree(cur);
    }
    (&vblocks->by_base)->rb_node = NULL;
    (&vblocks->by_size)->rb_node = NULL;
}

static inline __attribute__((always_inline)) 
void mapped_vblocks_cleanup(struct mapped_vblock_tree *mapped_vblocks, 
                            bool do_mapping_free, bool do_tlb_invl)
{
    struct mapped_vblock *cur, *tmp;
    struct rb_root *root;
    uint32_t *pte;
    uintptr_t va;
    size_t pa;

    root = &mapped_vblocks->by_base;
    rbtree_postorder_for_each_entry_safe(cur, tmp, root, by_base) {
        if (do_mapping_free) {
            pte = pte_from_va(cur->base);
            for (size_t i = 0; i < cur->size / PAGE_SIZE; i++) {
                
                if (!page_is_present(pte[i]))
                    continue;
                
                pa = pa_from_pte(pte[i]);
                
                if (page_is_shared(pa))
                    pgref_dec(pa);
                else
                    free_pages(pa, PAGE_SIZE);
                
                if (do_tlb_invl) {
                    va = cur->base + i*PAGE_SIZE;
                    tlb_invl(va);
                }
            }
        }
        kfree(cur);
    }
    root->rb_node = NULL;
}

static inline __attribute__((always_inline)) 
void pgdir_cleanup(bool do_recycle)
{
    uint32_t *pgdir, *pgtab;

    pgdir = pgdir_base();
    for (size_t i = 0; i < 768; i++) {
        if (pgdir[i]) {
            free_pages(pa_from_pde(pgdir[i], PAGE_SIZE), PAGE_SIZE);
            if (do_recycle) {
                pgdir[i] = 0;
                pgtab = pgtab_from_pdi(i);
                tlb_invl((uintptr_t)pgtab);
            }
        }
    }
}

/*
 * Cleanup routines for a user virtual address space.
 *
 * The cleanup pipeline is shared by exit(), exec() rollback, and fork()
 * rollback paths.  Each stage (vblock metadata, mapped regions, and PDE/PT)
 * can be selectively enabled via flags so that callers only perform the
 * work that is actually needed.
 *
 * Flags:
 *   CL_MAPPING_FREE:
 *      - Free physical pages belonging to mapped_vblocks.
 *      - Disabled during fork() rollback, because child mappings have not
 *        been COW-processed nor physically installed yet — only metadata
 *        nodes exist and freeing pages would be incorrect.
 *
 *   CL_TLB_INVL:
 *      - Invalidate TLB for unmapped pages.
 *      - Disabled during fork() rollback (we are in the parent context, so
 *        invalidating parent TLB entries for unmapped child ranges is wrong).
 *      - Disabled during exit(), because the process is about to lose its
 *        address space entirely (CR3 switch makes INVLPG unnecessary).
 *
 *   CL_RECYCLE:
 *      - Clear user PDE entries so that the pgdir structure can be reused
 *        for a fresh virtual address space.
 *      - Disabled during exit(), since the pgdir is not reused.
 *
 * All functions are static inline and are called with compile-time constant
 * flags, allowing the compiler to fully remove unused branches.  Each call
 * site therefore receives a specialized, zero-overhead cleanup sequence.
 */
static inline __attribute__((always_inline)) 
void user_vspace_cleanup(struct u_vblock_tree *vblocks, 
                         struct mapped_vblock_tree *mapped_vblocks, 
                         int flags)
{
    vblocks_cleanup(vblocks);
    mapped_vblocks_cleanup(mapped_vblocks,
                           flags & CL_MAPPING_FREE,
                           flags & CL_TLB_INVL);
    pgdir_cleanup(flags & CL_RECYCLE);
}

static inline void *tmp_map(size_t pa)
{
    uint32_t *pte;

    pte = pgtab_from_pdi(1022);
    while (*pte)
        pte++;
    *pte = pa | PG_RDWR | PG_PRESENT;
    return (void *)va_from_pte(pte);
}

static inline void tmp_unmap(void *mem)
{
    uint32_t *pte;

    pte = pte_from_va(mem);
    *pte = 0;
    tlb_invl((uintptr_t)mem);
}

static inline void cow_handle(uint32_t *pte, uintptr_t src)
{
    size_t origin_pa;
    size_t new_pa;
    uintptr_t src_base;
    uint32_t flags;
    void *dst;

    origin_pa = pa_from_pte(*pte);
    src_base = align_down(src, PAGE_SIZE);
    
    if (page_is_shared(origin_pa)) {
        new_pa = alloc_pages(PAGE_SIZE);
        
        dst = tmp_map(new_pa);
        memcpy32(dst, (void *)src_base, PAGE_SIZE / 4);
        tmp_unmap(dst);

        flags = (*pte & PTE_FLAGS_MASK) | PG_RDWR;
        *pte = new_pa | flags;

        pgref_dec(origin_pa);
    } else {
        *pte = (*pte & ~PG_COW_RDWR) | PG_RDWR;
    }
    
    tlb_invl(src_base);
}

#endif