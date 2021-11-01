#include "..\include\memory_pool.h"
#include "..\include\cache_size.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


inline uintptr_t _malloc_ref_internal(memory_pool* pool, uint32_t size);
inline void _free_ref_internal(memory_pool* pool, uintptr_t ptr);

/**
 * align byte boundary
 */
static inline size_t mpool_align(size_t siz) {
    return (siz + (POINTER_OFFSET - 1)) & ~(POINTER_OFFSET - 1);
}

/// <summary>
/// Initializes the memory pool according to the value provided
/// </summary>
/// <param name="pool_s">The pool s.</param>
/// <param name="amount_entries">The amount entries.</param>
/// <param name="ptr_size">Size of the PTR.</param>
/// <returns></returns>
/// <version author="Andre Cachopas" date="19/11/2020" version="1.0" machine="K-TOWER-WIN10"></version> 
int32_t init_pool(memory_pool* pool_s, uint32_t amount_entries, size_t ptr_size) {
    if (amount_entries == 0 || ptr_size == 0)
        return -1;

    size_t cache_size = CacheLineSize();
    if (pool_s == NULL)
        return -2;

    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    // compute the next highest power of 2 of 32-bit
    uint64_t padding = ptr_size + sizeof(struct entry_s);
    padding--;
    padding |= padding >> 1;
    padding |= padding >> 2;
    padding |= padding >> 4;
    padding |= padding >> 8;
    padding |= padding >> 16;
    padding++;
    pool_s->padded_size = padding;

    pool_s->block_size = ptr_size;
    pool_s->entries = amount_entries;
#ifdef _DEBUG
    pool_s->alloc_calls = 0;
    pool_s->free_calls = 0;
#endif
    pool_s->total_size = mpool_align(padding + ptr_size + sizeof(struct entry_s) * amount_entries *1);
    pool_s->cache_size = cache_size;

    // https://stackoverflow.com/questions/227897/how-to-allocate-aligned-memory-only-using-the-standard-library
    pool_s->storage_begin = _aligned_malloc(pool_s->total_size, padding);
    memset(&pool_s->storage_begin, 0, pool_s->total_size);

    pool_s->next_free_location = pool_s->storage_begin;
    pool_s->storage_end = pool_s->storage_begin + pool_s->total_size;

    return 0;
}

/// <summary>
/// Frees the memory allocated by the pool.
/// </summary>
/// <param name="pool">The pool.</param>
/// <version author="Andre Cachopas" date="19/11/2020" version="1.0" machine="K-TOWER-WIN10"></version> 
void free_pool(memory_pool* pool) {
    if (pool == NULL)
        return;

    if (pool->storage_begin != 0){
        _aligned_free(&pool->storage_begin);
        pool->storage_begin = NULL;
    }
    pool = NULL;
}

void reset_pool(memory_pool* pool) {
    if (pool == NULL)
        return;

    memset(&pool->storage_begin, 0, pool->total_size);
    pool->next_free_location = pool->storage_begin;
#if _DEBUG
    pool->alloc_calls = 0;
    pool->free_calls = 0;
#endif
}

/// <summary>
/// Switches to the next padded entry of the memory pool.
/// </summary>
/// <param name="pool">The pool.</param>
/// <param name="amount_requested">The amount requested.</param>
/// <version author="Andre Cachopas" date="19/11/2020" version="1.0" machine="K-TOWER-WIN10"></version> 
inline void switch_next_entry(memory_pool* pool, uint32_t amount_requested) {
    struct entry_s* temp = NULL;
    if (pool->next_free_location != 0 && (pool->next_free_location + amount_requested + sizeof(struct entry_s)) < pool->storage_end ) {
        uintptr_t position = (pool->next_free_location + amount_requested + sizeof(struct entry_s));
        //using padded memory should be optional.
        //temp = pool->next_free_location + pool->padded_size;
        temp = position;
    }

    if (temp == NULL){
        temp = pool->storage_begin;
    }
#if _DEBUG
    pool->alloc_calls++;
#endif
    if (temp->is_use == 0) {
        pool->next_free_location = temp;
    } else {
        // no more memory avail
        pool->next_free_location = NULL;
    }
}

/// <summary>
/// Internal function used to retrieve the next address position in the pool
/// </summary>
/// <param name="pool">The pool.</param>
/// <param name="size">The size.</param>
/// <returns></returns>
/// <version author="Andre Cachopas" date="19/11/2020" version="1.0" machine="K-TOWER-WIN10"></version> 
inline uintptr_t _malloc_ref_internal(memory_pool* pool, uint32_t size) {
    if (pool == NULL)
        return NULL;

    if (pool->next_free_location == 0) {
        //printf("No more memory available.\n");
        return NULL;
    }
    struct entry_s* mem = pool->next_free_location;
    mem->is_use = 1;
    mem->size = size;
    mem->ptr = mem + POINTER_OFFSET;
    switch_next_entry(pool, pool->block_size);
    return mem->ptr;
}

/// <summary>
/// Internal function used to released the allocate memory.
/// </summary>
/// <param name="pool">The pool.</param>
/// <param name="ptr">The PTR.</param>
/// <version author="Andre Cachopas" date="19/11/2020" version="1.0" machine="K-TOWER-WIN10"></version> 
inline void _free_ref_internal(memory_pool* pool, int8_t* ptr) {
    if (pool == NULL  || ptr == NULL)
        return;
    // we access directly to the ptr and change its state
    struct entry_s *entry = (struct entry_s*) (ptr - 2*pool->cache_size);
    entry->is_use = 0;
#if _DEBUG
    pool->free_calls++;
#endif
}

// External functions. 

#ifndef DISABLE_MEMORY_POOLING
uintptr_t malloc_ref(void* pool, size_t size) {
    return _malloc_ref_internal((memory_pool*)pool, size);
}

void free_ref(void* pool, uintptr_t ptr) {
    _free_ref_internal((memory_pool*) pool, ptr);
}
#endif


#ifdef __cplusplus
}
#endif
