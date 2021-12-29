//
// Created by trongminhle on 12/3/21.
//

#ifndef ASSIGNMENT_MEMORY_ALLOCATOR_TEST_H
#define ASSIGNMENT_MEMORY_ALLOCATOR_TEST_H

#define _DEFAULT_SOURCE
#include "mem.h"
#include "mem_internals.h"
#include "util.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


struct test_result { void *_function;};

struct test {
    struct test_result (*test_function)(void);
};

bool make_init_heap();

struct test_result test_1();
struct test_result test_2();
struct test_result test_3();
struct test_result test_4();
struct test_result test_5();


#endif //ASSIGNMENT_MEMORY_ALLOCATOR_TEST_H
