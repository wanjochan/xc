/**
 * External Test for XC Array Functionality
 * 
 * This test validates the array functionality through the public API.
 * It only uses functions and types defined in libxc.h.
 */
#include "test_utils.h"
#include <stdio.h>
#include <string.h>

/* Test basic array functionality */
void test_array_basic(void) {
    test_start("Array Basic Functionality (External)");
    
    printf("Testing basic array functionality through public API...\n");
    
    // Create an array
    xc_val arr = xc.new(XC_TYPE_ARRAY);
    TEST_ASSERT(arr != NULL, "Array creation failed");
    TEST_ASSERT(xc.is(arr, XC_TYPE_ARRAY), "Array type check failed");
    
    // Create some values to add to the array
    xc_val num1 = xc.new(XC_TYPE_NUMBER, 42.0);
    xc_val str1 = xc.new(XC_TYPE_STRING, "Hello");
    xc_val bool1 = xc.new(XC_TYPE_BOOL, 1);
    
    // Add elements to the array using push method
    xc.call(arr, "push", num1);
    xc.call(arr, "push", str1);
    xc.call(arr, "push", bool1);
    
    // Check array length
    xc_val length = xc.call(arr, "length");
    TEST_ASSERT(length != NULL, "Array length retrieval failed");
    
    // Get elements by index
    xc_val elem0 = xc.call(arr, "get", xc.new(XC_TYPE_NUMBER, 0.0));
    xc_val elem1 = xc.call(arr, "get", xc.new(XC_TYPE_NUMBER, 1.0));
    xc_val elem2 = xc.call(arr, "get", xc.new(XC_TYPE_NUMBER, 2.0));
    
    TEST_ASSERT(elem0 != NULL, "Element 0 retrieval failed");
    TEST_ASSERT(elem1 != NULL, "Element 1 retrieval failed");
    TEST_ASSERT(elem2 != NULL, "Element 2 retrieval failed");
    
    TEST_ASSERT(xc.is(elem0, XC_TYPE_NUMBER), "Element 0 should be a number");
    TEST_ASSERT(xc.is(elem1, XC_TYPE_STRING), "Element 1 should be a string");
    TEST_ASSERT(xc.is(elem2, XC_TYPE_BOOL), "Element 2 should be a boolean");
    
    // Pop an element
    xc_val popped = xc.call(arr, "pop");
    TEST_ASSERT(popped != NULL, "Pop operation failed");
    TEST_ASSERT(xc.is(popped, XC_TYPE_BOOL), "Popped element should be a boolean");
    
    // Check new length
    xc_val new_length = xc.call(arr, "length");
    TEST_ASSERT(new_length != NULL, "Array length retrieval after pop failed");
    
    printf("Array test completed successfully.\n");
    
    test_end("Array Basic Functionality (External)");
}

/* Test array advanced operations */
void test_array_advanced(void) {
    test_start("Array Advanced Operations (External)");
    
    printf("Testing advanced array operations through public API...\n");
    
    // Create an array
    xc_val arr = xc.new(XC_TYPE_ARRAY);
    
    // Add elements
    for (int i = 0; i < 5; i++) {
        xc_val num = xc.new(XC_TYPE_NUMBER, (double)i);
        xc.call(arr, "push", num);
    }
    
    // Test slice operation
    xc_val start = xc.new(XC_TYPE_NUMBER, 1.0);
    xc_val end = xc.new(XC_TYPE_NUMBER, 4.0);
    
    // 创建包含起始和结束索引的数组参数
    xc_val slice_args = xc.new(XC_TYPE_ARRAY);
    xc.call(slice_args, "push", start);
    xc.call(slice_args, "push", end);
    
    xc_val sliced = xc.call(arr, "slice", slice_args);
    
    TEST_ASSERT(sliced != NULL, "Slice operation failed");
    TEST_ASSERT(xc.is(sliced, XC_TYPE_ARRAY), "Slice result should be an array");
    
    xc_val slice_length = xc.call(sliced, "length");
    TEST_ASSERT(slice_length != NULL, "Sliced array length retrieval failed");
    
    // Test concat operation
    xc_val arr2 = xc.new(XC_TYPE_ARRAY);
    xc.call(arr2, "push", xc.new(XC_TYPE_NUMBER, 100.0));
    xc.call(arr2, "push", xc.new(XC_TYPE_NUMBER, 200.0));
    
    xc_val concat_result = xc.call(arr, "concat", arr2);
    TEST_ASSERT(concat_result != NULL, "Concat operation failed");
    TEST_ASSERT(xc.is(concat_result, XC_TYPE_ARRAY), "Concat result should be an array");
    
    xc_val concat_length = xc.call(concat_result, "length");
    TEST_ASSERT(concat_length != NULL, "Concatenated array length retrieval failed");
    
    // Test join operation
    xc_val separator = xc.new(XC_TYPE_STRING, ", ");
    xc_val joined = xc.call(arr, "join", separator);
    TEST_ASSERT(joined != NULL, "Join operation failed");
    TEST_ASSERT(xc.is(joined, XC_TYPE_STRING), "Join result should be a string");
    
    printf("Array advanced operations test completed successfully.\n");
    
    test_end("Array Advanced Operations (External)");
} 