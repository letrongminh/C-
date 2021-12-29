//
// Created by trongminhle on 12/3/21.
//

#include "test.h"

struct test_result out_res;
void *heap;
struct block_header *block;

bool make_init_heap() {
    heap = heap_init(20000);
    block = (struct block_header *) heap;
    return true;
}

struct test_result test_1() {
    printf(" test 1: normal successful memory allocation \n");
    debug_heap(stdout, block);
    void *mem = _malloc(1000);
    debug_heap(stdout, block);
    _free(mem);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_2() {
    printf(" test 2: freeing one block from several allocated \n");
    void *mem_1 = _malloc(1234);
    void *mem_2 = _malloc(5678);
    debug_heap(stdout, block);
    _free(mem_1);
    debug_heap(stdout, block);
    _free(mem_2);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_3() {
    printf(" test 3: release of the two blocks of several dedicated \n");
    void* mem_1 = _malloc(2000);
    void* mem_2 = _malloc(3000);
    void* mem_3 = _malloc(4000);
    debug_heap(stdout, block);
    _free(mem_1);
    _free(mem_2);
    debug_heap(stdout, block);
    _free(mem_3);
    printf("========================================\n\n");
    return out_res;
}

struct test_result test_4() {
    printf(" test 4: memory has run out, the new memory region expands the old one \n");
    debug_heap(stdout, block);
    void *mem_1 = _malloc(1234);
    void *mem_2 = _malloc(2345);
    debug_heap(stdout, block);
    _free(mem_1);
    _free(mem_2);
    printf("========================================\n\n");
    return out_res;
}

static inline void mmap_region(size_t length, void *addr) {

    size_t count = (size_t) addr / getpagesize();
    size_t remains = (size_t) addr % getpagesize();

    uint8_t *total = (uint8_t *) (getpagesize() * (count + (remains > 1)));

    mmap(total, length, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE, 0, 0);
}

struct test_result test_5() {
    printf(" test 5: memory has run out, the old memory region cannot be expanded due to a different allocated range of addresses, the new region is allocated in a different place \n");

    debug_heap(stdout, block);

    while (block->next != NULL)
        block = block->next;
    void *addr = block + block->capacity.bytes;

    // test cannot extend length
    mmap_region(3210, addr);

    // test cannot extend allocate
    void *allocated = _malloc(98765);
    debug_heap(stdout, block);
    _free(allocated);
    printf("========================================\n\n");
    return out_res;
}


