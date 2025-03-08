/**
 * External Test Main for XC Runtime
 * 
 * This file serves as the entry point for all external tests.
 * It includes all test functions from the individual test files.
 */
#include "test_utils.h"
#include <stdio.h>
#include <string.h>

/* Function declarations from other test files */
/* Array tests */
void test_array_basic(void);
void test_array_advanced(void);

/* Object tests */
void test_object_basic(void);
void test_object_prototype(void);

/* Function tests */
void test_function_basic(void);
void test_function_closure(void);

/* Exception tests */
void test_exception_basic(void);
void test_uncaught_exception(void);

/* Main function - entry point for all tests */
int main(void) {
    // Initialize test framework
    test_init("XC External Test Suite");

    // Register function tests
    //test_register("function_basic", test_function_basic, "function", "Test basic function functionality");
    //test_register("function_closure", test_function_closure, "function", "Test function closure functionality");
    
    // Register array tests
    //test_register("array_basic", test_array_basic, "array", "Test basic array functionality");
    test_register("array_advanced", test_array_advanced, "array", "Test advanced array operations");
    
    /*
    // Register exception tests
    //test_register("exception_basic", test_exception_basic, "exception", "Test basic exception handling");
    //test_register("uncaught_exception", test_uncaught_exception, "exception", "Test uncaught exception handling");
    
    // Register object tests
    //test_register("object_basic", test_object_basic, "object", "Test basic object functionality");
    //test_register("object_prototype", test_object_prototype, "object", "Test object prototype inheritance");
    */
    
    // Run all tests
    test_run_all();
    
    // Clean up
    test_cleanup();
    
    return 0;
} 