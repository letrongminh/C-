//
// Created by trongminhle on 12/5/21.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mem_internals.h"
#include "mem.h"
#include "test.h"

int main() {
    struct test tests[] = {
            {test_1 },
            {test_2 },
            {test_3 },
            {test_4 },
            {test_5 }};
    make_init_heap();
    for (size_t i = 0; i < 5; i++) {
        (*tests[i].test_function)();
    }
    return 0;
}
