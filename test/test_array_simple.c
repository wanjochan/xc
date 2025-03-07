/**
 * Simple Array Type Tests
 * 
 * This file contains basic tests for the XC array type implementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/xc/xc.h"
// #include "../src/xc/xc_gc.h"  // Removed since we've merged it into xc.c
#include "../src/xc/xc_internal.h"

// Global runtime instance
static xc_runtime_t *rt;

// Test array creation and basic properties
static void test_array_create() {
    printf("Testing array creation...\n");
    
    // Create an array
    xc_val array = xc.create(XC_TYPE_ARRAY);
    assert(array != NULL);
    assert(xc.type_of(array) == XC_TYPE_ARRAY);
    
    // Test array operations using the runtime interface
    xc_val num1 = xc.create(XC_TYPE_NUMBER, 10);
    xc_val num2 = xc.create(XC_TYPE_NUMBER, 20);
    xc_val num3 = xc.create(XC_TYPE_NUMBER, 30);
    
    // Call array methods
    xc.call(array, "push", num1);
    xc.call(array, "push", num2);
    xc.call(array, "push", num3);
    
    // Check array length
    int length = xc.call(array, "length");
    assert(length == 3);
    
    // Get elements
    xc_val item0 = xc.call(array, "get", 0);
    xc_val item1 = xc.call(array, "get", 1);
    xc_val item2 = xc.call(array, "get", 2);
    
    assert(xc.is(item0, XC_TYPE_NUMBER));
    assert(xc.is(item1, XC_TYPE_NUMBER));
    assert(xc.is(item2, XC_TYPE_NUMBER));
    
    // Pop an element
    xc_val popped = xc.call(array, "pop");
    assert(xc.is(popped, XC_TYPE_NUMBER));
    
    // Check new length
    length = xc.call(array, "length");
    assert(length == 2);
    
    // Clean up
    xc_release(array);
    xc_release(popped);
    
    printf("Array creation tests passed!\n");
}

// Test array operations
static void test_array_operations() {
    printf("Testing array operations...\n");
    
    // Create an array
    xc_val array = xc.create(XC_TYPE_ARRAY);
    
    // Test push/pop
    for (int i = 0; i < 5; i++) {
        xc_val num = xc.create(XC_TYPE_NUMBER, i * 10);
        xc.call(array, "push", num);
    }
    
    assert(xc.call(array, "length") == 5);
    
    // Test join
    xc_val separator = xc.create(XC_TYPE_STRING, ",");
    xc_val joined = xc.call(array, "join", separator);
    
    assert(xc.is(joined, XC_TYPE_STRING));
    
    // Test slice
    xc_val slice = xc.call(array, "slice", 1, 3);
    assert(xc.is(slice, XC_TYPE_ARRAY));
    assert(xc.call(slice, "length") == 2);
    
    // Clean up
    xc_release(array);
    xc_release(separator);
    xc_release(joined);
    xc_release(slice);
    
    printf("Array operations tests passed!\n");
}

// Run all array tests
void run_array_tests() {
    printf("\n=== Running Simple Array Tests ===\n\n");
    
    test_array_create();
    test_array_operations();
    
    printf("\n=== All Simple Array Tests Passed! ===\n\n");
}

// Main function
int main() {
    // Initialize XC runtime
    xc_init();
    
    // Run tests
    run_array_tests();
    
    // Shutdown XC runtime
    xc_shutdown();
    
    return 0;
} 