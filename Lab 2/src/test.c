//
// Created by trongminhle on 12/3/21.
//

#include "test.h"
#include "mem.h"
#include <unistd.h>

#include "mem_internals.h"
#include "util.h"


static struct block_header* block;
static struct block_header *block_get_header(void *contents) {
    return (struct block_header *) (((uint8_t *) contents) - offsetof(
            struct block_header, contents));
}


struct test_result out_res;
void *heap;
//struct block_header *block;

bool make_init_heap() {
    heap = heap_init(20000);
    block = (struct block_header *) heap;
    return true;
}

struct test_result test_1() {
    printf(" test 1: normal successful memory allocation \n");
    debug_heap(stdout, block);
    void *q = _malloc(500);
    debug_heap(stdout, block);
    _free(q);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_2() {
    printf(" test 2: freeing one block from several allocated \n");
    void *block_1 = _malloc(1234);
    void *block_2 = _malloc(5678);
    debug_heap(stdout, block);
    _free(block_1);
    debug_heap(stdout, block);
    _free(block_2);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_3() {
    printf(" test 3: release of the two blocks of several dedicated \n");
    void* block_1 = _malloc(2000);
    void* block_2 = _malloc(3000);
    void* block_3 = _malloc(4000);
    debug_heap(stdout, block);
    _free(block_1);
    _free(block_2);
    debug_heap(stdout, block);
    _free(block_3);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_4() {
    printf(" test 4: memory has run out, the new memory region expands the old one \n");
    const size_t SZ = 1000;
    debug_heap(stdout, block);
    //void *block_1 = _malloc(1234);
    //void *block_2 = _malloc(2345);
    void* mem = _malloc(SZ);
    debug_heap(stdout, block);


    //------------------new-------------------
    if (mem == NULL)
        err("Err: Null pointer.");
    struct block_header *last = block_get_header(mem);
    if(last->capacity.bytes != SZ)
        err("Ошибка при выделении памяти: размер выделенной памяти не соответствует запрошенному.");
    if(last->is_free)
        err("Ошибка при выделении памяти: блок не помечен как занятый.");
    _free(mem);
    if(!last->is_free)
        err("Ошибка при очистке памяти: блок не помечен как свободный.");
    //debug("\nПамять успешно очищена. Состояние кучи:\n");
    debug_heap(stderr, heap);



    //_free(block_1);
    //_free(block_2);
    printf("========================================\n\n");
    return out_res;
}

void mmap_aloc(size_t length, void *addr) {

    size_t count = (size_t) addr / getpagesize();
    size_t remains = (size_t) addr % getpagesize();

    uint8_t *total = (uint8_t *) (getpagesize() * (count + (remains > 1)));

    mmap(total, length, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE, 0, 0);
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
    const size_t SZ = 50000;
    struct block_header* iter = block;
    struct block_header* last;
    while(iter) {
        last = iter;
        iter = iter->next;
    }
    void* addr = (uint8_t*) last + size_from_capacity(last->capacity).bytes;

    addr = map_pages_for_main((uint8_t*) (getpagesize() * ((size_t) addr / getpagesize() +
                                                           (((size_t) addr % getpagesize()) > 0))), 1000,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE);

    /*
    addr = mmap( (uint8_t*) (getpagesize() * ((size_t) addr / getpagesize() +
                                              (((size_t) addr % getpagesize()) > 0))), 1000,
                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,0, 0);

    */
    printf("\nАдрес, по которому память стала недоступна: %p.\n", addr);
    void* mem = _malloc(SZ);
    struct block_header* new = block_get_header(mem);
    if (mem == NULL)
        err("err: Null pointer.");
    if(!new)
        err("Ошибка при выделении памяти: блок не встроен в кучу.");
    if(new->capacity.bytes != SZ)
        err("Ошибка при выделении памяти: размер выделенной памяти не соответствует запрошенному.");
    if(new->is_free)
        err("Ошибка при выделении памяти: блок не помечен как занятый.");
    debug_heap(stderr, heap);
    _free(mem);
    if(!new->is_free)
        err("Ошибка при очистке памяти: блок не помечен как свободный.");
    printf("\nПамять успешно очищена. Состояние кучи:\n");
    debug_heap(stderr, heap);



    printf("========================================\n\n");
    return out_res;
}


