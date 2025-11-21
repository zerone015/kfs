#include "kmalloc.h"
#include "vmm.h"
#include "panic.h"

static struct list_head free_list[FREE_LIST_MAX];

static void freelist_init(void)
{
    for (size_t i = 0; i < FREE_LIST_MAX; i++)
        init_list_head(free_list + i);
}

static size_t freelist_idx(size_t size)
{
    size_t i;

    i = -1;
    size /= MIN_SIZE;
    while (size) {
        i++;
        size /= 2;
    }
    return i;
}

static void chunk_split(struct malloc_chunk *chunk, size_t remainder_size)
{
    struct malloc_chunk *remainder_chunk;

    next_chunk(chunk)->prev_size = remainder_size & ~CHUNK_FLAGS;

    chunk->size -= remainder_size & ~CHUNK_FLAGS;

    remainder_chunk = next_chunk(chunk);
    remainder_chunk->size = remainder_size | PREV_INUSE;

    list_add(&remainder_chunk->list_head, free_list + freelist_idx(remainder_size));
}

/*
* Heap layout:  [ free_chunk ] → [ dummy1 ] → [ dummy2 ]
*
* Reason:
*   During coalescing, the allocator checks whether the “next chunk”
*   is in use by examining the PREV_INUSE bit stored in the *next
*   chunk’s next chunk*. (i.e., it may read up to two chunks ahead.)
*
*   Therefore the last real chunk in this heap region must be
*   followed by two valid dummy chunks. Without these dummies,
*   next_chunk() or the second-level next_chunk() would step outside
*   the region and cause an unintended page fault → kernel panic.
*/
static void heap_init(struct malloc_chunk *free_chunk, size_t size)
{
    struct malloc_chunk *dummy_chunk1, *dummy_chunk2;

    free_chunk->size = size - 2*MIN_SIZE;

    dummy_chunk1 = next_chunk(free_chunk);
    dummy_chunk1->size = MIN_SIZE; 

    dummy_chunk2 = next_chunk(dummy_chunk1);
    dummy_chunk2->size = PREV_INUSE;
}

static struct malloc_chunk *morecore(size_t size)
{
    struct malloc_chunk *free_chunk;
    int flags;

    if (is_aligned(size, PAGE_LARGE_SIZE))
        size += PAGE_LARGE_SIZE;
    else
        size = align_up(size, PAGE_LARGE_SIZE);

    free_chunk = (struct malloc_chunk *)vb_alloc(size);
    if (!free_chunk)
        return NULL;
        
    flags = IS_HEAD | PREV_INUSE;
    heap_init(free_chunk, size | flags);
    
    return free_chunk;
}

static struct malloc_chunk *chunk_from(size_t size)
{
    struct malloc_chunk *chunk = NULL;
    size_t idx;

    idx = freelist_idx(size);
    if (idx >= FREE_LIST_MAX)
        return NULL;

    for (size_t i = idx + 1; i < FREE_LIST_MAX; i++) {
        if (!list_empty(free_list + i)) {
            chunk = list_first_entry(free_list + i, struct malloc_chunk, list_head);
            list_del(&chunk->list_head);
            break;
        }
    }

    return chunk;
}

void hmm_init(uintptr_t start)
{
    struct malloc_chunk *free_chunk;
    size_t size;
    uintptr_t end;
    size_t remaining;
    size_t dummy_area_size;
    int flags;

    freelist_init();

     /* start: leftover space unused by the physical memory manager's metadata */
    start = align_up(start, 4);
    end = align_up(start, PAGE_LARGE_SIZE);

    dummy_area_size = MIN_SIZE * 2;
    remaining = end - start;

    /* need: 2 dummy chunks + minimum free chunk */
    if (remaining < MIN_SIZE + dummy_area_size) {
        size = PAGE_LARGE_SIZE;
        free_chunk = vb_alloc(size);
        if (!free_chunk)
            do_panic("failed to allocate initial heap region");
    } else {
        size = remaining;
        free_chunk = (struct malloc_chunk *)start;
    }

    flags = PREV_INUSE;
    heap_init(free_chunk, size | flags);

    list_add(&free_chunk->list_head, free_list + freelist_idx(free_chunk->size));
}

size_t ksize(void *mem)
{
    struct malloc_chunk *chunk;
    size_t payload_size;

    chunk = mem2chunk(mem);
    payload_size = chunk_size(chunk) - SIZE_SZ;
    return payload_size;
}

void *kmalloc(size_t size)
{
    struct malloc_chunk *chunk;
    size_t remainder_size;

    size = request_size(size);

    chunk = chunk_from(size);
    if (!chunk) {
        if (!(chunk = morecore(size)))
            return NULL;
    }

    remainder_size = chunk->size - size;
    if (remainder_size >= MIN_SIZE)
        chunk_split(chunk, remainder_size);
    else
        set_inuse(chunk);

    return chunk2mem(chunk);
}

void kfree(void *mem)
{
    struct malloc_chunk *chunk;

    if (!mem)
        return;

    chunk = mem2chunk(mem);

    if (!next_is_inuse(chunk)) {
        list_del(&next_chunk(chunk)->list_head);
        chunk->size += chunk_size(next_chunk(chunk));
    }

    if (!prev_is_inuse(chunk)) {
        list_del(&prev_chunk(chunk)->list_head);
        prev_chunk(chunk)->size += chunk_size(chunk);
        chunk = prev_chunk(chunk);
    }

    if (is_freeable_vb(chunk)) {
        vb_free(chunk);
        return;
    } 

    clear_inuse(chunk);
    next_chunk(chunk)->prev_size = chunk_size(chunk);
    
    list_add(&chunk->list_head, free_list + freelist_idx(chunk->size));
}
