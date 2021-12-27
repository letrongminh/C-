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
