#include <stdarg.h>
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

void debug_block(struct block_header* b, const char* fmt, ... );
//void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );


extern inline bool region_is_invalid(const struct region *r);

static size_t pages_count(block_size query_size) {
    return query_size.bytes / getpagesize() + ((query_size.bytes % getpagesize()) > 0);
}

static size_t round_pages(block_size query_size) { return getpagesize() * pages_count(query_size); }

static struct region region_init(void *restrict addr, block_size region_size) {
    return (struct region) {.addr = addr, .size = region_size.bytes, .extends = false};
}


static void block_init(void *restrict addr, const block_capacity block_capacity, void *restrict next) {
    struct block_header *restrict const block_header_ptr = (struct block_header *) addr;

    *block_header_ptr = (struct block_header) {
            .next = next,
            .capacity = block_capacity,
            .is_free = true
    };
}


static block_size region_actual_size(const block_size query_size) {
    return size_max(round_pages(query_size), REGION_MIN_SIZE);
}


static void *map_pages(void const *const addr, const size_t length, const int additional_flags) {
    return mmap((void *) addr,
                length,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | additional_flags,
                0,
                0);
}


static struct region alloc_region(void const *const addr, const block_size query_size) {

    const block_size actual_region_size = region_actual_size(query_size);
    const block_capacity region_block_capacity = capacity_from_size(actual_region_size);

    void *mapped_page_ptr = map_pages(addr, actual_region_size.bytes, MAP_FIXED_NOREPLACE);
    if (mapped_page_ptr == MAP_FAILED)
        mapped_page_ptr = map_pages(addr, actual_region_size.bytes, 0);
    if (mapped_page_ptr == MAP_FAILED)
        return REGION_INVALID;

    block_init(mapped_page_ptr, region_block_capacity, NULL);

    return region_init(mapped_page_ptr, actual_region_size);
}

static void *block_after(struct block_header const *block);


void *heap_init(const size_t initial) {

    const struct region region = alloc_region(HEAP_START, (block_size) {initial});

    return (region_is_invalid(&region)) ? NULL : region.addr;
}


#define BLOCK_MIN_CAPACITY 24

static bool block_splittable(const struct block_header *const restrict block, const block_capacity query_capacity) {

    return block->is_free && ((size_from_capacity((block_capacity)
    {query_capacity.bytes + BLOCK_MIN_CAPACITY})).bytes <= block->capacity.bytes);
}

static bool split_if_too_big(struct block_header *restrict const block, const block_capacity query_capacity) {

    if (block_splittable(block, query_capacity)) {
        const block_size second_block_size = (block_size) {block->capacity.bytes - query_capacity.bytes};

        block->capacity = query_capacity;

        void *restrict const second_block_ptr = block_after(block);

        block_init(second_block_ptr, capacity_from_size(second_block_size), block->next);
        block->next = second_block_ptr;
        return true;
    }
    return false;
}


/*  --- Слияние соседних свободных блоков --- */

static void *block_after(struct block_header const *const block) {
    return (void *) (block->contents + block->capacity.bytes);
}

static bool blocks_continuous(
        struct block_header const *const fst,
        struct block_header const *const snd) {
    return (void *) snd == block_after(fst);
}

static bool mergeable(struct block_header const *restrict fst, struct block_header const *restrict snd) {
    return fst->is_free && snd->is_free && blocks_continuous(fst, snd);
}


static bool try_merge_with_next(struct block_header *restrict const block) {

    if (block->next) {

        struct block_header *const restrict fst = block;
        struct block_header *restrict snd = fst->next;

        if (mergeable(block, block->next)) {
            fst->next = snd->next;
            fst->capacity.bytes += size_from_capacity(snd->capacity).bytes;

            return true;
        }
    }
    return false;
}


/*  --- Если размера кучи хватает --- */

struct block_search_result {
    enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
    struct block_header *block;
};


static struct block_search_result
find_good_or_last(struct block_header *restrict block, const block_capacity query_capacity) {

    while (block->next != NULL) {
        while (try_merge_with_next(block));
        if (block_splittable(block, query_capacity))
            return (struct block_search_result) {.type = BSR_FOUND_GOOD_BLOCK, .block = block};
        block = block->next;
    }

    return (block_splittable(block, query_capacity))
           ? (struct block_search_result) {.type = BSR_FOUND_GOOD_BLOCK, .block = block}
           : (struct block_search_result) {.type = BSR_REACHED_END_NOT_FOUND, .block = block};
}


static struct block_search_result
try_memalloc_existing(struct block_header *restrict block, const block_capacity query_capacity) {

    struct block_search_result splittable_or_last_block = find_good_or_last(block, query_capacity);
    if (splittable_or_last_block.type != BSR_FOUND_GOOD_BLOCK) return splittable_or_last_block;
    split_if_too_big(splittable_or_last_block.block, query_capacity);

    return splittable_or_last_block;
}


static struct block_header *grow_heap(struct block_header *restrict last, const block_size query_size) {

    void *new_region_adr = block_after(last);
    const struct region new_region = alloc_region(new_region_adr, query_size);

    if (region_is_invalid(&new_region)) return NULL;
    last->next = new_region.addr;
    if (try_merge_with_next(last)) {
        return last;
    }
    return last->next;
}

static struct block_header *
memalloc(struct block_header *restrict const heap_start, const block_capacity query_capacity) {
    struct block_search_result block_search_result = try_memalloc_existing(heap_start, query_capacity);
    if (block_search_result.type) {
        const block_size query_size = size_from_capacity(query_capacity);
        struct block_header *new_block = grow_heap(block_search_result.block, query_size);
        if (new_block == NULL) return NULL;
        block_search_result = try_memalloc_existing(heap_start, query_capacity);
    }

    block_search_result.block->is_free = false;
    return block_search_result.block;
}

void *_malloc(const size_t query) {
    const block_capacity query_capacity = (block_capacity) {query};
    struct block_header *restrict const addr = memalloc((struct block_header *) HEAP_START, query_capacity);
    if (addr) return addr->contents;
    else return NULL;
}

static struct block_header *block_get_header(const void *const contents) {
    return (struct block_header *) (((uint8_t *) contents) - offsetof(struct block_header, contents));
}

void _free(void *const mem) {
    if (!mem) return;
    struct block_header *header = block_get_header(mem);
    header->is_free = true;
    while (try_merge_with_next(header));
}
