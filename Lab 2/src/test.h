//
// Created by trongminhle on 12/3/21.
//

/*
#ifndef ASSIGNMENT_MEMORY_ALLOCATOR_TEST_H
#define ASSIGNMENT_MEMORY_ALLOCATOR_TEST_H

#include <stdio.h>
#include <mem.h>


void test_1(void *heap);
void test_2(void *heap);
void test_3(void *heap);
void test_4(void *heap);
void test_5(void *heap);

#endif
*/
//=======================================
#ifndef TEST_H
#define TEST_H

#define _DEFAULT_SOURCE
#include "mem.h"
#include "mem_internals.h"
#include "util.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


struct test_result {
    const char *message;
};


struct test {
    struct test_result (*test_function)(void);
};

bool prepare_test_env();

struct test_result test_1();

struct test_result test_2();

struct test_result test_3();

struct test_result test_4();

struct test_result test_5();

void test_all();

#endif //TEST_H
