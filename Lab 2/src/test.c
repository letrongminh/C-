//
// Created by trongminhle on 12/3/21.
//

#include "test.h"
void debug(const char* fmt, ... );
static struct block_header* block;
static struct block_header *block_get_header(void *contents) {
    return (struct block_header *) (((uint8_t *) contents) - offsetof(
            struct block_header, contents));
}


static void extra_end(char *err_msg, void *heap) {
    debug_heap(stderr, heap);
    //debug_line(stderr);
    err(err_msg);
}
struct test_result out_res;
void *heap;

bool make_init_heap() {
    heap = heap_init(12000);
    block = (struct block_header *) heap;
    return true;
}

//static void* block_all_ed;
struct test_result test_1() {
    printf(" test 1: normal successful memory allocation \n");
    debug_heap(stderr, block);
    void *data1 = _malloc(2000);
    struct block_header *data1_header = block_get_header(data1);
/*
    if (data1 == NULL) extra_end("_malloc returned NULL!", block);
    if (data1_header->is_free == true) extra_end("_malloc returned free block!", block);
    if (data1_header->next == NULL) extra_end("_malloc returned not linked block!", block);
    if (data1_header->capacity.bytes != 2000) extra_end("_malloc returned block with wrong capacity", block);
*/
    debug_heap(stderr, block);

    _free(data1);

    if (data1_header->is_free == false) extra_end("_free didn't free block!", block);

    _free(data1);
    debug_heap(stderr, block);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_2() {
    printf(" test 2: freeing one block from several allocated \n");
    debug_heap(stderr, block);
    void *data1 = _malloc(2000);
    void *data2 = _malloc(2000);
    //if (data1 == NULL || data2 == NULL) extra_end("_malloc returned NULL!", block);

    debug_heap(stderr, block);

    struct block_header *data1_header = block_get_header(data1);
    struct block_header *data2_header = block_get_header(data2);

    _free(data2);

    if (data1_header->is_free == true) extra_end("_free free extra block!", block);
    if (data2_header->is_free == false) extra_end("_free didn't free block!", block);

    debug_heap(stderr, block);

    _free(data2);
    _free(data1);
    debug_heap(stderr, block);
    printf("========================================\n\n");
    return out_res;
}


struct test_result test_3() {
    printf(" test 3: release of the two blocks of several dedicated \n");
    void *data1 = _malloc(2000);
    void *data2 = _malloc(3000);
    void *data3 = _malloc(4000);
    if (data1 == NULL || data2 == NULL || data3 == NULL) extra_end("_malloc returned NULL!", block);

    debug_heap(stderr, block);

    struct block_header *data1_header = block_get_header(data1);
    struct block_header *data2_header = block_get_header(data2);
    struct block_header *data3_header = block_get_header(data3);

    if (data1_header->capacity.bytes != 2000
        || data2_header->capacity.bytes != 3000
        || data3_header->capacity.bytes != 4000)
        extra_end("_malloc returned block with wrong capacity!", block);

    _free(data2);

    //if (data2_header->is_free == false) extra_end("_free didn't free block!", block);

    debug_heap(stderr, block);

    _free(data1);

    //if (data1_header->is_free == false) extra_end("_free didn't free block!", block);
    //if (data1_header->next != data3_header) extra_end("_free isn't merge free blocks!", block);

    debug_heap(stderr, block);

    _free(data3);
    _free(data2);
    _free(data1);

    debug_heap(stderr, block);

    printf("========================================\n\n");
    return out_res;
}

static void *block_after(struct block_header const *const block_) {
    return (void *) (block_->contents + block_->capacity.bytes);
}

struct test_result test_4() {
    printf(" test 4: memory has run out, the new memory region expands the old one \n");

    debug_heap(stderr, block);

    void *data1 = _malloc(10000);
    void *data2 = _malloc(6000);
    void *data3 = _malloc(5000);

    if (data1 == NULL || data2 == NULL || data3 == NULL) extra_end("_malloc returned NULL!", block);

    debug_heap(stderr, block);

    struct block_header *data1_header = block_get_header(data1);
    struct block_header *data2_header = block_get_header(data2);
    struct block_header *data3_header = block_get_header(data3);

    if (data1_header->next != data2_header || data2_header->next != data3_header)
        extra_end("_malloc returned not linked blocks", block);
/*
    if (data1_header->capacity.bytes != 10000
        || data2_header->capacity.bytes != 6000
        || data3_header->capacity.bytes != 5000)
        extra_end("_malloc returned block with wrong capacity!", block);
*/
    if (block_after(data1_header) != data2_header || block_after(data2_header) != data3_header)
        extra_end("_malloc returned non-sequentially placed blocks", block);

    _free(data3);
    _free(data2);
    _free(data1);

    debug_heap(stderr, block);
    printf("========================================\n\n");
    return out_res;
}
/*
static inline void mmap_region(size_t length, void *addr) {
    size_t count = (size_t) addr / getpagesize();
    size_t remains = (size_t) addr % getpagesize();

    uint8_t *total = (uint8_t *) (getpagesize() * (count + (remains > 1)));

    mmap(total, length, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE| MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}
*/
struct test_result test_5() {
    printf(" test 5: memory has run out, the new region is allocated in a different place \n");
    debug_heap(stderr, block);
    
    void *region_between_start_adr = block_after(block);

    debug("bet_reg :%10p \n", region_between_start_adr);

    void *adr = mmap(region_between_start_adr,
                     50000,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     0,
                     0);

    debug("bet_reg adr:%10p \n", adr);

    void *data1 = _malloc(10000);
    void *data2 = _malloc(20000);

    if (data1 == NULL || data2 == NULL) extra_end("_malloc returned NULL!", block);

    debug_heap(stderr, block);

    struct block_header *data1_header = block_get_header(data1);
    struct block_header *data2_header = block_get_header(data2);

    debug("first_next :%10p \n", data1_header->next);
    debug("second :%10p \n", data2_header);
/*
    if (data1_header->next == data2_header)
        extra_end("_malloc missed block between additional region and first block", block);

    if (data1_header->capacity.bytes != 10000
        || data2_header->capacity.bytes != 20000)
        extra_end("_malloc returned block with wrong capacity!", block);

    if (block_after(data1_header) == data2_header)
        extra_end("_malloc ignore between region when grow", block);
*/
    _free(data2);
    _free(data1);
    debug_heap(stderr, block);
    return out_res;

}
