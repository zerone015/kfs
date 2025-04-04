#include "hmm.h"
#include "vmm.h"

static struct list_head free_list[FREE_LIST_MAX];

static inline void __freelist_init(void)
{
    for (size_t i = 0; i < FREE_LIST_MAX; i++)
        init_list_head(free_list + i);
}

static inline size_t __freelist_idx(size_t size)
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

static inline void __chunk_split(struct malloc_chunk *chunk, size_t remainder_size)
{
    struct malloc_chunk *remainder_chunk;

    __next_chunk(chunk)->prev_size = remainder_size & ~CHUNK_FLAGS;
    chunk->size -= remainder_size & ~CHUNK_FLAGS;
    remainder_chunk = __next_chunk(chunk);
    remainder_chunk->size = remainder_size | PREV_INUSE;
    list_add(&remainder_chunk->list_head, free_list + __freelist_idx(remainder_size));
}

static inline void __heap_init(struct malloc_chunk *free_chunk, size_t size)
{
    struct malloc_chunk *dummy_chunk1, *dummy_chunk2;

    free_chunk->size = size;

    dummy_chunk1 = __next_chunk(free_chunk);
    dummy_chunk1->size = MIN_SIZE; 

    dummy_chunk2 = __next_chunk(dummy_chunk1);
    dummy_chunk2->size = PREV_INUSE;
}

static inline struct malloc_chunk *__morecore(size_t size)
{
    struct malloc_chunk *free_chunk;
    size_t free_chunk_size;

    if (is_align_kpage(size))
        size += K_PAGE_SIZE;
    else
        size = align_kpage(size);
    free_chunk = (struct malloc_chunk *)vb_alloc(size);
    if (!free_chunk)
        return NULL;
    free_chunk_size = size - 2*MIN_SIZE;
    __heap_init(free_chunk, free_chunk_size | IS_HEAD | PREV_INUSE);
    return free_chunk;
}

static inline struct malloc_chunk *__find_fit_chunk(size_t size)
{
    struct malloc_chunk *chunk = NULL;
    size_t idx;

    idx = __freelist_idx(size);
    if (idx >= FREE_LIST_MAX)
        return NULL;
    for (size_t i = idx + 1; i < FREE_LIST_MAX; i++) {
        if (!list_empty(free_list + i)) {
            chunk = list_first_entry(free_list + i, struct malloc_chunk, list_head);
            list_del(&chunk->list_head);
            break;
        }
    }
    if (!chunk) {
        list_for_each_entry(chunk, free_list + idx, list_head) {
            if (size <= chunk->size) {
                list_del(&chunk->list_head);
                break;
            }
        }
        if (list_entry_is_head(chunk, free_list + idx, list_head))
            chunk = __morecore(size);
    }
    return chunk;
}

void hmm_init(uintptr_t mem)
{
    struct malloc_chunk *free_chunk;
    size_t free_chunk_size;

    __freelist_init();
    free_chunk = (struct malloc_chunk *)mem;
    free_chunk_size = align_kpage(mem) - mem - 2*MIN_SIZE;
    __heap_init(free_chunk, free_chunk_size | PREV_INUSE);
    list_add(&free_chunk->list_head, free_list + __freelist_idx(free_chunk->size));
}

size_t ksize(void *mem)
{
    return __chunk_size(__mem2chunk(mem)) - SIZE_SZ;
}

void *kmalloc(size_t size)
{
    struct malloc_chunk *chunk;
    size_t remainder_size;

    size = __request_size(size);
    chunk = __find_fit_chunk(size);
    if (!chunk)
        return NULL;
    remainder_size = chunk->size - size;
    if (remainder_size >= MIN_SIZE)
        __chunk_split(chunk, remainder_size);
    else
        __set_inuse(chunk);
    return __chunk2mem(chunk);
}

void kfree(void *mem)
{
    struct malloc_chunk *chunk;

    if (!mem)
        return;
    chunk = __mem2chunk(mem);
    if (!__next_is_inuse(chunk)) {
        list_del(&__next_chunk(chunk)->list_head);
        chunk->size += __chunk_size(__next_chunk(chunk));
    }
    if (!__prev_is_inuse(chunk)) {
        list_del(&__prev_chunk(chunk)->list_head);
        __prev_chunk(chunk)->size += __chunk_size(chunk);
        chunk = __prev_chunk(chunk);
    }
    if (__is_freeable_vb(chunk)) {
        vb_free(chunk);
        return;
    } 
    __clear_inuse(chunk);
    __next_chunk(chunk)->prev_size = __chunk_size(chunk);
    list_add(&chunk->list_head, free_list + __freelist_idx(chunk->size));
}
