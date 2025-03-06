/*
 * test_xc.c - XC Runtime Tests Main Entry
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xc.h"
#include "test_utils.h"

/* External test suite declarations */
void test_xc_types(void);
void test_xc_gc(void);
void test_xc_exception(void);
void register_composite_type_tests(void);
void register_object_tests(void);
void register_stdc_tests(void);

/* Simple print function for debugging */
static void print_val(xc_val val) {
    if (!val) {
        printf("null");
        return;
    }
    int type = xc.type_of(val);
    printf("Object(type=%d)", type);
}

/* Test runtime interface availability */
static void test_runtime_interface(void) {
    test_start("Runtime Interface");
    
    TEST_ASSERT_NOT_NULL(xc.alloc_object, "alloc_object interface available");
    TEST_ASSERT_NOT_NULL(xc.type_of, "type_of interface available");
    TEST_ASSERT_NOT_NULL(xc.create, "create interface available");
    TEST_ASSERT_NOT_NULL(xc.is, "is interface available");
    TEST_ASSERT_NOT_NULL(xc.call, "call interface available");
    TEST_ASSERT_NOT_NULL(xc.gc, "gc interface available");
    TEST_ASSERT_NOT_NULL(xc.try_catch_finally, "exception handling available");
    
    test_end("Runtime Interface");
}

/* Register all test suites */
static void register_test_suites(void) {
    test_register("runtime.interface", test_runtime_interface, "core", 
                 "Test runtime interface availability");
}

int main(int argc, char* argv[]) {
    /* Initialize test framework */
    test_init("XC Runtime Test Suite");
    
    /* Register test suites */
    register_test_suites();
    
    /* Run type system tests */
    test_xc_types();
    
    /* Run garbage collection tests */
    test_xc_gc();
    
    /* Run exception handling tests */
    test_xc_exception();
    
    /* Register and run composite type tests */
    register_composite_type_tests();
    
    /* Register object tests */
    register_object_tests();
    
    /* Register standard library tests */
    register_stdc_tests();
    
    /* Run all registered tests */
    test_run_all();
    
    /* Show test summary */
    test_summary();
    
    return g_test_suite.failures > 0 ? 1 : 0;
}
