//
// Created by trongminhle on 12/3/21.
//

#include "test.h"



static struct block_header* block;
static struct block_header *block_get_header(void *contents) {
    return (struct block_header *) (((uint8_t *) contents) - offsetof(
            struct block_header, contents));
}


struct test_result out_res;
void *heap;

bool make_init_heap() {
    heap = heap_init(10000);
    block = (struct block_header *) heap;
    return true;
}

static void* block_all_ed;
struct test_result test_1() {
    printf(" test 1: normal successful memory allocation \n");
    debug_heap(stdout, block);
    void *q = _malloc(500);
    debug_heap(stdout, block);
    _free(q);
    debug_heap(stderr, block);
    //block_all_ed = q;
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_2() {
    printf(" test 2: freeing one block from several allocated \n");
    debug_heap(stdout, block);
    void *block_1 = _malloc(1234);
    void *block_2 = _malloc(5678);
    debug_heap(stdout, block);
    _free(block_1);
    //debug_heap(stdout, block);
    //_free(block_2);
    block_all_ed = block_2;
    debug_heap(stderr, block);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_3() {
    printf(" test 3: release of the two blocks of several dedicated \n");
    //void* block_1 = _malloc(2000);
    void* block_2 = _malloc(3000);
    //void* block_3 = _malloc(4000);
    debug_heap(stdout, block);
    //_free(block_1);
    _free(block_all_ed);
    _free(block_2);
    debug_heap(stdout, block);
    //_free(block_3);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_4() {
    printf(" test 4: memory has run out, the new memory region expands the old one \n");

    const size_t mem_size_init = 25000;
    debug_heap(stdout, block);

    void* mem_block = _malloc(mem_size_init);
    debug_heap(stdout, block);
    struct block_header *last = block_get_header(mem_block);

    if(last->capacity.bytes != mem_size_init)
        err("Error: the size of the allocated memory does not match the requested one.");
    if(last->is_free)
        err("Error: block not marked as occupied.");

    _free(mem_block);
    if(!last->is_free)
        err("Error: block is not marked as free.");

    debug_heap(stderr, heap);

    printf("========================================\n\n");
    return out_res;
}

static inline void mmap_region(size_t length, void *addr) {
    size_t count = (size_t) addr / getpagesize();
    size_t remains = (size_t) addr % getpagesize();

    uint8_t *total = (uint8_t *) (getpagesize() * (count + (remains > 1)));

    mmap(total, length, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE| MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

struct test_result test_5() {
    printf(" test 5: memory has run out, the new region is allocated in a different place \n");
    debug_heap(stdout, block);
    const size_t mem_size_cre = 100000;
    const size_t leng_ = 1000;

/*
    const size_t mem_size_init = 50000;
    struct block_header* temp = block;
    struct block_header* last;
    while(temp) {
        last = temp;
        temp = temp->next;
    }
    void* addr = (uint8_t*) last + size_from_capacity(last->capacity).bytes;


    addr = (uint8_t*) (getpagesize() * ((size_t) addr / getpagesize() + (((size_t) addr % getpagesize()) > 1)));


    mmap(addr, 100000, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_FIXED, 0, 0);

    void* mem_block = _malloc(mem_size_init);
    debug_heap(stderr, heap);


    struct block_header* new_block = block_get_header(mem_block);
    if(new_block->capacity.bytes != mem_size_init)
        err("Error: the size of the allocated memory does not match the requested one.");
    if(new_block->is_free)
        err("Memory allocation error: the block is not marked as occupied.");

    _free(mem_block);
    debug_heap(stderr, heap);
*/

    while (block->next != NULL)
        block = block->next;

    void *addr = block + block->capacity.bytes;
    mmap_region(leng_, addr);

    void *allocated = _malloc(mem_size_cre);

    if ((uint8_t *) block->next == (uint8_t *) allocated - offsetof(struct block_header, contents)) {
        printf("New region allocated in the old region\n");
    }

    _free(allocated);

    debug_heap(stderr, block);
    return out_res;
}


