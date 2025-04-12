#include "hmm.h"
#include "vmm.h"

static struct list_head free_list[FREE_LIST_MAX];

static inline void freelist_init(void)
{
    for (size_t i = 0; i < FREE_LIST_MAX; i++)
        init_list_head(free_list + i);
}

static inline size_t freelist_idx(size_t size)
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

static inline void chunk_split(struct malloc_chunk *chunk, size_t remainder_size)
{
    struct malloc_chunk *remainder_chunk;

    next_chunk(chunk)->prev_size = remainder_size & ~CHUNK_FLAGS;
    chunk->size -= remainder_size & ~CHUNK_FLAGS;
    remainder_chunk = next_chunk(chunk);
    remainder_chunk->size = remainder_size | PREV_INUSE;
    list_add(&remainder_chunk->list_head, free_list + freelist_idx(remainder_size));
}

static inline void heap_init(struct malloc_chunk *free_chunk, size_t size)
{
    struct malloc_chunk *dummy_chunk1, *dummy_chunk2;

    free_chunk->size = size;

    dummy_chunk1 = next_chunk(free_chunk);
    dummy_chunk1->size = MIN_SIZE; 

    dummy_chunk2 = next_chunk(dummy_chunk1);
    dummy_chunk2->size = PREV_INUSE;
}

static inline struct malloc_chunk *morecore(size_t size)
{
    struct malloc_chunk *free_chunk;
    size_t free_chunk_size;

    if (is_align_4mb_page(size))
        size += K_PAGE_SIZE;
    else
        size = align_4mb_page(size);
    free_chunk = (struct malloc_chunk *)vb_alloc(size);
    if (!free_chunk)
        return NULL;
    free_chunk_size = size - 2*MIN_SIZE;
    heap_init(free_chunk, free_chunk_size | IS_HEAD | PREV_INUSE);
    return free_chunk;
}

static inline struct malloc_chunk *find_fit_chunk(size_t size)
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

void hmm_init(uintptr_t mem)
{
    struct malloc_chunk *free_chunk;
    size_t free_chunk_size;

    freelist_init();
    free_chunk = (struct malloc_chunk *)align_4byte(mem);
    free_chunk_size = align_4mb_page(mem) - align_4byte(mem) - 2*MIN_SIZE;
    heap_init(free_chunk, free_chunk_size | PREV_INUSE);
    list_add(&free_chunk->list_head, free_list + freelist_idx(free_chunk->size));
}

size_t ksize(void *mem)
{
    return chunk_size(mem2chunk(mem)) - SIZE_SZ;
}

void *kmalloc(size_t size)
{
    struct malloc_chunk *chunk;
    size_t remainder_size;

    size = request_size(size);
    chunk = find_fit_chunk(size);
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
