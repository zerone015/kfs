#ifndef _KMALLOC_H
#define _KMALLOC_H

#include "list.h"
#include "paging.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Chunk layout and metadata reuse
 *
 * - Allocated chunk:
 *      Only `size` is meaningful. Both `prev_size` and `list_head`
 *      belong to the user payload region.
 *
 * - Free chunk:
 *      The payload region is reused as:
 *         [ list_head ]           (free-list linkage)
 *         [ tail copy of size ]   (stored in prev_size field)
 *
 *      NOTE:
 *        The `prev_size` field in this struct is *not* part of this
 *        chunk’s logical header. It is a "tail copy" of this chunk’s
 *        own size, placed at the end of the chunk so that the *next*
 *        chunk can locate this chunk by reading its prev_size.
 *
 *      In other words:
 *         - `size` is the official header
 *         - `prev_size` is a backward navigation aid for the *next chunk*
 *         - space for `prev_size` is taken from the payload to avoid
 *           inflating this chunk’s header size.
 *
 *      Since the minimum chunk size is >= 16 bytes, the payload area
 *      always has enough room to store both free-list metadata
 *      (`list_head`) and the tail `prev_size`.
 */
struct malloc_chunk {
    size_t prev_size;
    size_t size;
    struct list_head list_head;
};

#define SIZE_SZ             sizeof(size_t)
#define MAX_CHUNK_SIZE      (K_VSPACE_SIZE / 2)
#define FREE_LIST_MAX       (__builtin_ctz(MAX_CHUNK_SIZE / MIN_SIZE) + 1)  /* index 0 starts at MIN_SIZE */
#define MALLOC_ALIGNMENT    SIZE_SZ
#define MALLOC_ALIGN_MASK   (MALLOC_ALIGNMENT - 1)
#define MIN_CHUNK_SIZE      sizeof(struct malloc_chunk)
#define MIN_SIZE            ((MIN_CHUNK_SIZE + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#define PREV_INUSE          0x1
#define IS_HEAD             0x2
#define CHUNK_FLAGS         (IS_HEAD | PREV_INUSE)

#define chunk_size(p)     ((p)->size & ~CHUNK_FLAGS)
#define next_chunk(p)     ((struct malloc_chunk *)(((uint8_t *)(p)) + chunk_size(p)))
#define prev_chunk(p)     ((struct malloc_chunk *)(((uint8_t *)(p)) - ((p)->prev_size)))
#define chunk_is_head(p)  ((p)->size & IS_HEAD)
#define is_inuse(p)       ((next_chunk(p))->size & PREV_INUSE)
#define prev_is_inuse(p)  ((p)->size & PREV_INUSE)
#define next_is_inuse(p)  ((is_inuse(next_chunk(p))))
#define set_inuse(p)      ((next_chunk(p))->size |= PREV_INUSE)
#define clear_inuse(p)    ((next_chunk(p))->size &= ~PREV_INUSE)
#define request_size(sz)  \
    (((sz) + SIZE_SZ + MALLOC_ALIGN_MASK < MIN_SIZE) ?               \
    MIN_SIZE :                                                      \
    ((sz) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#define is_freeable_vb(p) (chunk_is_head(p) && (chunk_size(p) + 2*MIN_SIZE) == vb_size(p))
#define chunk2mem(p)      ((void *)((uint8_t *)(p) + 2*SIZE_SZ))
#define mem2chunk(mem)    ((struct malloc_chunk *)((uint8_t *)(mem) - 2*SIZE_SZ))

void hmm_init(uintptr_t mem);
void *kmalloc(size_t size);
void kfree(void *mem);
size_t ksize(void *mem);

#endif