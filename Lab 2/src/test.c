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
    //_free(q);

    block_all_ed = q;
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_2() {
    printf(" test 2: freeing one block from several allocated \n");
    void *block_1 = _malloc(1234);
    //void *block_2 = _malloc(5678);
    debug_heap(stdout, block);
    _free(block_1);
    debug_heap(stdout, block);
    //_free(block_2);
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

struct test_result test_5() {
    printf(" test 5: memory has run out, the old memory region cannot be expanded due to a different allocated range of addresses, the new region is allocated in a different place \n");
    /*
    debug_heap(stdout, block);

    while (block->next != NULL)
        block = block->next;

    //void *addr = block + block->capacity.bytes;

    //mmap_aloc(3210, addr);


    map_pages_for_main((uint8_t*)heap+4*getpagesize(),3210,MAP_PRIVATE | MAP_ANONYMOUS );

    void *block_5 = _malloc(98765);
    debug_heap(stdout, block);
    _free(block_5);
     */
    const size_t mem_size_init = 60000;
    struct block_header* temp = block;
    struct block_header* last;
    while(temp) {
        last = temp;
        temp = temp->next;
    }
    void* addr = (uint8_t*) last + size_from_capacity(last->capacity).bytes;

    /*
    addr = map_pages_for_main((uint8_t*) (getpagesize() * ((size_t) addr / getpagesize() +
                                                           (((size_t) addr % getpagesize()) > 0))), 1000,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE);
    */

    map_pages_for_main((uint8_t*) (getpagesize() * ((size_t) addr / getpagesize() +
                                        (((size_t) addr % getpagesize()) > 0))), 1000,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE);

    //printf("\nThe address at which the memory became unavailable: %p.\n", addr);
    void* mem_block = _malloc(mem_size_init);
    struct block_header* new_block = block_get_header(mem_block);


    if(new_block->capacity.bytes != mem_size_init)
        err("Memory allocation error: the size of the allocated memory does not match the requested one.");

    if(new_block->is_free)
        err("Memory allocation error: the block is not marked as occupied.");

    debug_heap(stderr, heap);
    _free(mem_block);

    if(!new_block->is_free)
        err("An error occurred while clearing memory: the block was not marked as free.");

    debug_heap(stderr, heap);

    return out_res;
}


