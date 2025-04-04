#ifndef _HMM_H
#define _HMM_H

#include "list.h"
#include "paging.h"
#include <stddef.h>
#include <stdint.h>

struct malloc_chunk {
    size_t prev_size;
    size_t size;
    struct list_head list_head;
};

#define SIZE_SZ             sizeof(size_t)
#define FREE_LIST_MAX       (__builtin_ctz(K_VSPACE_SIZE / 2 / MIN_SIZE) + 1)
#define MALLOC_ALIGNMENT    SIZE_SZ
#define MALLOC_ALIGN_MASK   (MALLOC_ALIGNMENT - 1)
#define MIN_CHUNK_SIZE      sizeof(struct malloc_chunk)
#define MIN_SIZE            ((MIN_CHUNK_SIZE + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#define PREV_INUSE          0x1
#define IS_HEAD             0x2
#define CHUNK_FLAGS         (IS_HEAD | PREV_INUSE)

#define __chunk_size(p)     ((p)->size & ~CHUNK_FLAGS)
#define __next_chunk(p)     ((struct malloc_chunk *)(((uint8_t *)(p)) + __chunk_size(p)))
#define __prev_chunk(p)     ((struct malloc_chunk *)(((uint8_t *)(p)) - ((p)->prev_size)))
#define __chunk_is_head(p)  ((p)->size & IS_HEAD)
#define __is_inuse(p)       ((__next_chunk(p))->size & PREV_INUSE)
#define __prev_is_inuse(p)  ((p)->size & PREV_INUSE)
#define __next_is_inuse(p)  ((__is_inuse(__next_chunk(p))))
#define __set_inuse(p)      ((__next_chunk(p))->size |= PREV_INUSE)
#define __clear_inuse(p)    ((__next_chunk(p))->size &= ~PREV_INUSE)
#define __request_size(sz)  \
    (((sz) + SIZE_SZ + MALLOC_ALIGN_MASK < MIN_SIZE) ?               \
    MIN_SIZE :                                                      \
    ((sz) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#define __is_freeable_vb(p) (__chunk_is_head(p) && (__chunk_size(p) + 2*MIN_SIZE) == vb_size(p))
#define __chunk2mem(p)      ((void *)((uint8_t *)(p) + 2*SIZE_SZ))
#define __mem2chunk(mem)    ((struct malloc_chunk *)((uint8_t *)(mem) - 2*SIZE_SZ))

extern void hmm_init(uintptr_t mem);
extern void *kmalloc(size_t size);
extern void kfree(void *mem);
extern size_t ksize(void *mem);

#endif