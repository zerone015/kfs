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

static inline struct malloc_chunk *__find_chunk(size_t size)
{
    struct malloc_chunk *chunk = NULL;
    size_t idx;

    idx = __freelist_idx(size);
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
            chunk = NULL;
    }
    return chunk;
}

static inline void __chunk_split(struct malloc_chunk *chunk, size_t size)
{
    size_t remainder_size;
    struct malloc_chunk *next_chunk, *remainder_chunk;

    remainder_size = __chunk_size(chunk) - size;
    if (remainder_size >= MIN_SIZE) {
        next_chunk = __next_chunk(chunk);
        next_chunk->prev_size = remainder_size;
        chunk->size -= remainder_size;
        remainder_chunk = __next_chunk(chunk);
        remainder_chunk->size = remainder_size | PREV_INUSE;
        list_add(&remainder_chunk->list_head, free_list + __freelist_idx(remainder_size));
    }
}

static inline void __heap_init(struct malloc_chunk *free_chunk, size_t size)
{
    struct malloc_chunk *dummy_chunk1, *dummy_chunk2;

    free_chunk->size = size;
    list_add(&free_chunk->list_head, free_list + __freelist_idx(free_chunk->size));

    dummy_chunk1 = __next_chunk(free_chunk);
    dummy_chunk1->size = MIN_SIZE; 

    dummy_chunk2 = __next_chunk(dummy_chunk1);
    dummy_chunk2->size = PREV_INUSE;
}

static inline int __morecore(void)
{
    uintptr_t mem;
    size_t freechunk_size;

    mem = (uintptr_t)vs_alloc(K_PAGE_SIZE);
    if (!mem)
        return MORECORE_FAILURE;
    freechunk_size = K_PAGE_SIZE - 2*MIN_SIZE;
    __heap_init((struct malloc_chunk *)mem, freechunk_size | PREV_INUSE);
    return MORECORE_SUCCESS;
}

void hmm_init(uintptr_t mem)
{
    size_t freechunk_size;

    __freelist_init();
    freechunk_size = align_kpage(mem) - mem - 2*MIN_SIZE;
    __heap_init((struct malloc_chunk *)mem, freechunk_size | PREV_INUSE);
}

void *kmalloc(size_t size)
{
    struct malloc_chunk *chunk;

    size = __request_size(size);
    chunk = __find_chunk(size);
    if (!chunk) {
        if (__morecore() == MORECORE_FAILURE)
            return NULL;
        chunk = list_first_entry(free_list + MORECORE_LINDEX, struct malloc_chunk, list_head);
        list_del(&chunk->list_head);
    }
    __chunk_split(chunk, size);
    return __chunk2mem(chunk);
}