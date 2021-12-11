//
// Created by trongminhle on 12/3/21.
//

#include "memory_test.h"

int getpagesize(void);

void test_one(void*heap){
    printf("Page size is = %d\n",getpagesize());
    fprintf(stdout,"Test №1 : normal successful memory allocation\n");
    debug_heap(stdout, heap);
    void *q = _malloc(25);
    debug_heap(stdout, heap);
    _free(q);
    printf("\n\n");
}
void test_two(void *heap){
    fprintf(stdout,"Test №2 : freeing one block from several allocated blocks\n");

    //fprintf(stdout,"Lets see current heap status\n");

    debug_heap(stdout,heap);

    //fprintf(stdout,"now lets make some block\n");
    void *block_one=_malloc(65);

    //fprintf(stdout,"Lets see current heap status\n");
    debug_heap(stdout,heap);

    //fprintf(stdout,"Lets freeing block_one\n");
    _free(block_one);
    debug_heap(stdout,heap);
    printf("\n\n");
}
void test_three(void *heap){
    fprintf(stdout,"Test №3 : release of the two blocks of several dedicated\n");
    void *block_two=_malloc(66);
    void *block_three=_malloc(67);

    //fprintf(stdout,"Lets see current heap status\n");

    debug_heap(stdout,heap);

    //fprintf(stdout,"Lets freeing block_one and bloc_two\n");

    _free(block_three);
    _free(block_two);
    debug_heap(stdout,heap);
    printf("\n\n");
}
void test_four(void *heap){
    fprintf(stdout,"Test №4 : the memory has run out, the new memory region expands the old one\n");
    debug_heap(stdout,heap);
    fprintf(stdout,"add some extra \n");
    void*f=_malloc(9000);

    //fprintf(stdout,"Lets see current heap status\n");

    debug_heap(stdout,heap);
    _free(f);
    debug_heap(stdout,heap);
    printf("\n\n");
}
void test_five(void *heap){
    fprintf(stdout,"Test №5 \n");
    debug_heap(stdout,heap);
    map_pages_for_main((uint8_t*)heap+4*getpagesize(),88888888,MAP_FIXED);
    debug_heap(stdout,heap);
    _malloc(20422 );
    debug_heap(stdout,heap);
    _malloc(10000);
    debug_heap(stdout,heap);
}