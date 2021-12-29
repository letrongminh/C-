/*
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mem_internals.h"
#include "mem.h"
#include "test.h"

int main(){
    void* heap = heap_init(12345);
    test_1(heap);
    test_2(heap);
    test_3(heap);
    test_4(heap);
    test_5(heap);
    return 0;
}
*/

//==============================
#include "test.h"

int main() {
    //test_all();
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
    return 0;
}
