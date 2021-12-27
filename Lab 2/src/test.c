//
// Created by trongminhle on 12/3/21.
//

// #include "memory_test.h"
#include "test.h"

int getpagesize(void);


void test_1(void*heap){
    printf("Page size is = %d\n",getpagesize());
    printf("========================================\n\n");
    fprintf(stdout,"Test №1 : normal successful memory allocation\n");
    debug_heap(stdout, heap);
    void *q = _malloc(500);
    debug_heap(stdout, heap);
    _free(q);
    printf("========================================\n\n");
}
void test_2(void *heap){
    fprintf(stdout,"Test №2 : Freeing one block from several allocated\n");

    debug_heap(stdout,heap);

    void *block_one=_malloc(150);

    debug_heap(stdout,heap);

    _free(block_one);
    debug_heap(stdout,heap);
    printf("\n\n");
}
void test_3(void *heap){
    fprintf(stdout,"Test №3 : release of the two blocks of several dedicated\n");
    void *block_two=_malloc(200);
    void *block_three=_malloc(250);

    debug_heap(stdout,heap);

    _free(block_three);
    _free(block_two);
    debug_heap(stdout,heap);
    printf("\n\n");
}
void test_4(void *heap){
    fprintf(stdout,"Test №4 : the memory has run out, the new memory region expands the old one\n");
    debug_heap(stdout,heap);



    fprintf(stdout,"add some extra \n");
    void* f=_malloc(10000);

    debug_heap(stdout,heap);
    _free(f);
    debug_heap(stdout,heap);
    printf("========================================\n\n");
}
void test_5(void *heap){
    fprintf(stdout,"Test №5 : The memory has run out, the old memory region cannot be expanded due to a "
                   "different allocated range of addresses, the new region is allocated in a different place.\n");
    debug_heap(stdout,heap);

    // mapping fixed
    map_pages_for_main((uint8_t*)heap+4*getpagesize(),50000000,MAP_FIXED);
    debug_heap(stdout,heap);
    _malloc(98765 );
    debug_heap(stdout,heap);
    _malloc(43210);
    debug_heap(stdout,heap);
}
