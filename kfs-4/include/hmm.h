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
#define MORECORE_FAILURE    -1
#define MORECORE_SUCCESS    0
#define MORECORE_LINDEX     (K_PAGE_SHIFT - 1)

#define __chunk_size(p)     ((p)->size & ~PREV_INUSE)
#define __next_chunk(p)     ((struct malloc_chunk *)(((uint8_t *)(p)) + __chunk_size(p)))
#define __set_inuse(p)      ((__next_chunk(p))->size |= PREV_INUSE)
#define __clear_inuse(p)    ((__next_chunk(p))->size &= ~PREV_INUSE)
#define __request_size(sz)  \
    (((sz) + SIZE_SZ + MALLOC_ALIGN_MASK < MIN_SIZE) ?               \
    MIN_SIZE :                                                      \
    ((sz) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#define __chunk2mem(p)      ((void *)((uint8_t *)(p) + 2*SIZE_SZ))
#define __mem2chunk(mem)    ((struct malloc_chunk *)((uint8_t *)(mem) - 2*SIZE_SZ))

extern void hmm_init(uintptr_t mem);
extern void *kmalloc(size_t size);

#endif