//
// Created by trongminhle on 12/3/21.
//

/*
#include "test.h"

int getpagesize(void);


void test_1(void*heap){
    printf("Page size is = %d\n",getpagesize());
    printf("========================================\n\n");
    printf("Test №1 : normal successful memory allocation\n");
    debug_heap(stdout, heap);
    void *q = _malloc(500);
    debug_heap(stdout, heap);
    _free(q);
    printf("========================================\n\n");
}
void test_2(void *heap){
    printf("Test №2 : Freeing one block from several allocated\n");

    //debug_heap(stdout,heap);

    void *block_one=_malloc(150);

    debug_heap(stdout,heap);

    _free(block_one);
    debug_heap(stdout,heap);
    printf("\n\n");
}
void test_3(void *heap){
    printf("Test №3 : release of the two blocks of several dedicated\n");
    void *block_two=_malloc(200);
    void *block_three=_malloc(250);

    debug_heap(stdout,heap);

    printf("Free blocks\n");
    _free(block_three);
    _free(block_two);
    debug_heap(stdout,heap);
    printf("\n\n");
}

void test_4(void *heap){

    printf("Test №4 : the memory has run out, the new memory region expands the old one\n");

    debug_heap(stdout,heap);
    printf("add some extra \n");
    void* f=_malloc(10000);

    debug_heap(stdout,heap);
    _free(f);
    debug_heap(stdout,heap);
    printf("========================================\n\n");

}


//======================================================================

void test_5(void *heap){
    printf("Test №5 : The memory has run out, the old memory region cannot be expanded due to a "
                   "different allocated range of addresses, the new region is allocated in a different place.\n");
    debug_heap(stdout,heap);

    // mapping fixed
    map_pages_for_main((uint8_t*)heap+4*getpagesize(),50000000,MAP_FIXED);
    debug_heap(stdout,heap);
    _malloc(23456);
    debug_heap(stdout,heap);
    _malloc(98765);
    debug_heap(stdout,heap);
}
*/
//=================================================================

#include "test.h"

struct test_result success_test_result = {
        .message = "Done"
};

void *heap;
struct block_header *block;

bool prepare_test_env() {
    heap = heap_init(20000);
    block = (struct block_header *) heap;
    return true;
}


struct test_result test_1() {
    printf(" test 1 \n");
    debug_heap(stdout, block);
    void *mem = _malloc(1000);
    debug_heap(stdout, block);
    _free(mem);
    return success_test_result;
}


struct test_result test_2() {
    printf(" test 2 \n");
    void *mem_1 = _malloc(2000);
    void *mem_2 = _malloc(3000);
    debug_heap(stdout, block);
    _free(mem_1);
    debug_heap(stdout, block);
    _free(mem_2);
    return success_test_result;
}


struct test_result test_3() {
    printf(" test 3 \n");
    void* mem_1 = _malloc(2000);
    void* mem_2 = _malloc(3000);
    void* mem_3 = _malloc(4000);
    debug_heap(stdout, block);
    _free(mem_1);
    _free(mem_2);
    debug_heap(stdout, block);
    _free(mem_3);
    return success_test_result;
}

struct test_result test_4() {
    printf(" test 4 \n");
    debug_heap(stdout, block);
    void *mem_1 = _malloc(5000);
    void *mem_2 = _malloc(2000);
    debug_heap(stdout, block);
    _free(mem_1);
    _free(mem_2);
    return success_test_result;
}

static inline void mmap_region(size_t length, void *addr) {

    size_t count = (size_t) addr / getpagesize();
    size_t remains = (size_t) addr % getpagesize();

    uint8_t *total = (uint8_t *) (getpagesize() * (count + (remains > 1)));

    mmap(total, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}


struct test_result test_5() {
    printf(" test 5 \n");

    debug_heap(stdout, block);

    while (block->next != NULL)
        block = block->next;

    void *addr = block + block->capacity.bytes;

    // test cannot extend length
    mmap_region(1000, addr);

    // test cannot extend allocate
    void *allocated = _malloc(10000);

    debug_heap(stdout, block);

    _free(allocated);

    return success_test_result;
}
/*
void test_all() {
    struct test tests[] = {
            {test_1 },
            {test_2 },
            {test_3 },
            {test_4 },
            {test_5 }};
    prepare_test_env();
    for (size_t i = 0; i < 5; i++) {
        (*tests[i].test_function)();
    }
}
*/
