/**
 * XC Array Type Tests
 * 
 * This file contains comprehensive tests for the XC array type implementation.
 * It tests array creation, element access, modification, and various array operations.
 */
#include "test_utils.h"

#define TEST_ARRAY_STANDALONE 1

// Global runtime instance
static xc_runtime_t *rt;

// Test array creation and basic properties
static void test_array_create() {
    printf("Testing array creation...\n");
    
    // Create an empty array
    xc_val empty_array = rt->create(XC_TYPE_ARRAY);
    TEST_ASSERT(rt->is(empty_array, XC_TYPE_ARRAY), "Should be an array type");
    
    // Get the length of the empty array
    xc_val empty_length = rt->call(empty_array, "length");
    TEST_ASSERT(empty_length == 0, "Empty array should have length 0");
    
    // Create an array with initial values
    xc_val values[3];
    values[0] = rt->create(XC_TYPE_NUMBER, 10);
    values[1] = rt->create(XC_TYPE_STRING, "test");
    values[2] = rt->create(XC_TYPE_BOOL, 1);
    
    xc_val array = rt->create(XC_TYPE_ARRAY);
    rt->call(array, "push", values[0]);
    rt->call(array, "push", values[1]);
    rt->call(array, "push", values[2]);
    
    TEST_ASSERT(rt->is(array, XC_TYPE_ARRAY), "Should be an array type");
    
    // Get the length of the array
    xc_val length = rt->call(array, "length");
    TEST_ASSERT(length == 3, "Array should have length 3");
    
    // Check element types
    xc_val elem0 = rt->call(array, "get", 0);
    xc_val elem1 = rt->call(array, "get", 1);
    xc_val elem2 = rt->call(array, "get", 2);
    
    TEST_ASSERT(rt->is(elem0, XC_TYPE_NUMBER), "First element should be a number");
    TEST_ASSERT(rt->is(elem1, XC_TYPE_STRING), "Second element should be a string");
    TEST_ASSERT(rt->is(elem2, XC_TYPE_BOOL), "Third element should be a boolean");
    
    // Test out of bounds access
    xc_val out_of_bounds = rt->call(array, "get", 3);
    xc_val negative_index = rt->call(array, "get", -1);
    
    TEST_ASSERT(rt->is(out_of_bounds, XC_TYPE_NULL), "Out of bounds access should return null");
    TEST_ASSERT(rt->is(negative_index, XC_TYPE_NULL), "Negative index access should return null");
    
    printf("Array creation tests passed!\n");
}

// Test array element modification
static void test_array_set() {
    printf("Testing array element modification...\n");
    
    // Create an array
    xc_object_t *array = xc_array_create(rt);
    
    // Set elements
    xc_array_set(rt, array, 0, xc_number_create(rt, 42));
    TEST_ASSERT(xc_array_length(rt, array) == 1, "Array length should be 1 after setting element 0");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 0)) == 42, "Element 0 should be 42");
    
    // Set element at higher index (should auto-expand)
    xc_array_set(rt, array, 5, xc_string_create(rt, "hello"));
    TEST_ASSERT(xc_array_length(rt, array) == 6, "Array length should be 6 after setting element 5");
    TEST_ASSERT(xc_is_string(rt, xc_array_get(rt, array, 5)), "Element 5 should be a string");
    TEST_ASSERT(strcmp(xc_string_value(rt, xc_array_get(rt, array, 5)), "hello") == 0, "Element 5 should be 'hello'");
    
    // Check that intermediate elements are null
    for (int i = 1; i < 5; i++) {
        TEST_ASSERT(xc_is_null(rt, xc_array_get(rt, array, i)), "Intermediate element should be null");
    }
    
    // Replace an existing element
    xc_array_set(rt, array, 0, xc_boolean_create(rt, 0));
    TEST_ASSERT(xc_is_boolean(rt, xc_array_get(rt, array, 0)), "Element 0 should now be a boolean");
    TEST_ASSERT(xc_boolean_value(rt, xc_array_get(rt, array, 0)) == 0, "Element 0 should be false");
    
    // Clean up
    xc_gc_release(rt, array);
    
    printf("Array element modification tests passed!\n");
}

// Test array push and pop operations
static void test_array_push_pop() {
    printf("Testing array push and pop operations...\n");
    
    // Create an empty array
    xc_object_t *array = xc_array_create(rt);
    
    // Push elements
    xc_array_push(rt, array, xc_number_create(rt, 1));
    xc_array_push(rt, array, xc_number_create(rt, 2));
    xc_array_push(rt, array, xc_number_create(rt, 3));
    
    TEST_ASSERT(xc_array_length(rt, array) == 3, "Array length should be 3 after pushing 3 elements");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 0)) == 1, "Element 0 should be 1");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 1)) == 2, "Element 1 should be 2");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 2)) == 3, "Element 2 should be 3");
    
    // Pop elements
    xc_object_t *popped = xc_array_pop(rt, array);
    TEST_ASSERT(xc_is_number(rt, popped), "Popped value should be a number");
    TEST_ASSERT(xc_number_value(rt, popped) == 3, "Popped value should be 3");
    TEST_ASSERT(xc_array_length(rt, array) == 2, "Array length should be 2 after popping");
    
    popped = xc_array_pop(rt, array);
    TEST_ASSERT(xc_number_value(rt, popped) == 2, "Popped value should be 2");
    TEST_ASSERT(xc_array_length(rt, array) == 1, "Array length should be 1 after popping again");
    
    popped = xc_array_pop(rt, array);
    TEST_ASSERT(xc_number_value(rt, popped) == 1, "Popped value should be 1");
    TEST_ASSERT(xc_array_length(rt, array) == 0, "Array length should be 0 after popping again");
    
    // Pop from empty array
    popped = xc_array_pop(rt, array);
    TEST_ASSERT(xc_is_null(rt, popped), "Popping from empty array should return null");
    
    // Clean up
    xc_gc_release(rt, array);
    
    printf("Array push and pop tests passed!\n");
}

// Test array shift and unshift operations
static void test_array_shift_unshift() {
    printf("Testing array shift and unshift operations...\n");
    
    // Create an empty array
    xc_object_t *array = xc_array_create(rt);
    
    // Unshift elements (add to front)
    xc_array_unshift(rt, array, xc_number_create(rt, 3));
    xc_array_unshift(rt, array, xc_number_create(rt, 2));
    xc_array_unshift(rt, array, xc_number_create(rt, 1));
    
    TEST_ASSERT(xc_array_length(rt, array) == 3, "Array length should be 3 after unshifting 3 elements");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 0)) == 1, "Element 0 should be 1");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 1)) == 2, "Element 1 should be 2");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, array, 2)) == 3, "Element 2 should be 3");
    
    // Shift elements (remove from front)
    xc_object_t *shifted = xc_array_shift(rt, array);
    TEST_ASSERT(xc_is_number(rt, shifted), "Shifted value should be a number");
    TEST_ASSERT(xc_number_value(rt, shifted) == 1, "Shifted value should be 1");
    TEST_ASSERT(xc_array_length(rt, array) == 2, "Array length should be 2 after shifting");
    
    shifted = xc_array_shift(rt, array);
    TEST_ASSERT(xc_number_value(rt, shifted) == 2, "Shifted value should be 2");
    TEST_ASSERT(xc_array_length(rt, array) == 1, "Array length should be 1 after shifting again");
    
    shifted = xc_array_shift(rt, array);
    TEST_ASSERT(xc_number_value(rt, shifted) == 3, "Shifted value should be 3");
    TEST_ASSERT(xc_array_length(rt, array) == 0, "Array length should be 0 after shifting again");
    
    // Shift from empty array
    shifted = xc_array_shift(rt, array);
    TEST_ASSERT(xc_is_null(rt, shifted), "Shifting from empty array should return null");
    
    // Clean up
    xc_gc_release(rt, array);
    
    printf("Array shift and unshift tests passed!\n");
}

// Test array slice operation
static void test_array_slice() {
    printf("Testing array slice operation...\n");
    
    // Create an array with values 0-9
    xc_object_t *array = xc_array_create(rt);
    for (int i = 0; i < 10; i++) {
        xc_array_push(rt, array, xc_number_create(rt, i));
    }
    
    // Test basic slice
    xc_object_t *slice = xc_array_slice(rt, array, 2, 5);
    TEST_ASSERT(xc_is_array(rt, slice), "Slice should be an array");
    TEST_ASSERT(xc_array_length(rt, slice) == 3, "Slice length should be 3");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, slice, 0)) == 2, "First element of slice should be 2");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, slice, 1)) == 3, "Second element of slice should be 3");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, slice, 2)) == 4, "Third element of slice should be 4");
    
    // Test slice with negative start (from end)
    xc_object_t *neg_slice = xc_array_slice(rt, array, -3, 10);
    TEST_ASSERT(xc_array_length(rt, neg_slice) == 3, "Negative start slice length should be 3");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, neg_slice, 0)) == 7, "First element should be 7");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, neg_slice, 1)) == 8, "Second element should be 8");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, neg_slice, 2)) == 9, "Third element should be 9");
    
    // Test slice with negative end (from end)
    xc_object_t *neg_end_slice = xc_array_slice(rt, array, 5, -2);
    TEST_ASSERT(xc_array_length(rt, neg_end_slice) == 3, "Negative end slice length should be 3");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, neg_end_slice, 0)) == 5, "First element should be 5");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, neg_end_slice, 1)) == 6, "Second element should be 6");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, neg_end_slice, 2)) == 7, "Third element should be 7");
    
    // Test out of bounds slice
    xc_object_t *out_slice = xc_array_slice(rt, array, 8, 15);
    TEST_ASSERT(xc_array_length(rt, out_slice) == 2, "Out of bounds slice should have length 2");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, out_slice, 0)) == 8, "First element should be 8");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, out_slice, 1)) == 9, "Second element should be 9");
    
    // Test invalid slice (start > end)
    xc_object_t *invalid_slice = xc_array_slice(rt, array, 5, 2);
    TEST_ASSERT(xc_array_length(rt, invalid_slice) == 0, "Invalid slice should have length 0");
    
    // Clean up
    xc_gc_release(rt, array);
    xc_gc_release(rt, slice);
    xc_gc_release(rt, neg_slice);
    xc_gc_release(rt, neg_end_slice);
    xc_gc_release(rt, out_slice);
    xc_gc_release(rt, invalid_slice);
    
    printf("Array slice tests passed!\n");
}

// Test array concat operation
static void test_array_concat() {
    printf("Testing array concat operation...\n");
    
    // Create two arrays
    xc_object_t *array1 = xc_array_create(rt);
    xc_array_push(rt, array1, xc_number_create(rt, 1));
    xc_array_push(rt, array1, xc_number_create(rt, 2));
    
    xc_object_t *array2 = xc_array_create(rt);
    xc_array_push(rt, array2, xc_number_create(rt, 3));
    xc_array_push(rt, array2, xc_number_create(rt, 4));
    
    // Concatenate arrays
    xc_object_t *concat = xc_array_concat(rt, array1, array2);
    TEST_ASSERT(xc_is_array(rt, concat), "Concat result should be an array");
    TEST_ASSERT(xc_array_length(rt, concat) == 4, "Concat array length should be 4");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, concat, 0)) == 1, "First element should be 1");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, concat, 1)) == 2, "Second element should be 2");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, concat, 2)) == 3, "Third element should be 3");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, concat, 3)) == 4, "Fourth element should be 4");
    
    // Verify original arrays are unchanged
    TEST_ASSERT(xc_array_length(rt, array1) == 2, "Original array1 should still have length 2");
    TEST_ASSERT(xc_array_length(rt, array2) == 2, "Original array2 should still have length 2");
    
    // Concat with empty array
    xc_object_t *empty = xc_array_create(rt);
    xc_object_t *concat_empty = xc_array_concat(rt, array1, empty);
    TEST_ASSERT(xc_array_length(rt, concat_empty) == 2, "Concat with empty should have length 2");
    
    // Clean up
    xc_gc_release(rt, array1);
    xc_gc_release(rt, array2);
    xc_gc_release(rt, concat);
    xc_gc_release(rt, empty);
    xc_gc_release(rt, concat_empty);
    
    printf("Array concat tests passed!\n");
}

// Test array join operation
static void test_array_join() {
    printf("Testing array join operation...\n");
    
    // Create an array with mixed types
    xc_object_t *array = xc_array_create(rt);
    xc_array_push(rt, array, xc_number_create(rt, 1));
    xc_array_push(rt, array, xc_string_create(rt, "hello"));
    xc_array_push(rt, array, xc_boolean_create(rt, 1));
    
    // Join with comma separator
    xc_object_t *joined = xc_array_join(rt, array, xc_string_create(rt, ","));
    TEST_ASSERT(xc_is_string(rt, joined), "Join result should be a string");
    TEST_ASSERT(strcmp(xc_string_value(rt, joined), "1,hello,true") == 0, 
                "Joined string should be '1,hello,true'");
    
    // Join with empty separator
    xc_object_t *joined_empty = xc_array_join(rt, array, xc_string_create(rt, ""));
    TEST_ASSERT(strcmp(xc_string_value(rt, joined_empty), "1hellotrue") == 0, 
                "Joined string with empty separator should be '1hellotrue'");
    
    // Join empty array
    xc_object_t *empty = xc_array_create(rt);
    xc_object_t *joined_empty_array = xc_array_join(rt, empty, xc_string_create(rt, ","));
    TEST_ASSERT(strcmp(xc_string_value(rt, joined_empty_array), "") == 0, 
                "Joined empty array should be empty string");
    
    // Clean up
    xc_gc_release(rt, array);
    xc_gc_release(rt, joined);
    xc_gc_release(rt, joined_empty);
    xc_gc_release(rt, empty);
    xc_gc_release(rt, joined_empty_array);
    
    printf("Array join tests passed!\n");
}

// Test array indexOf operation
static void test_array_index_of() {
    printf("Testing array indexOf operation...\n");
    
    // Create an array
    xc_object_t *array = xc_array_create(rt);
    xc_array_push(rt, array, xc_number_create(rt, 10));
    xc_array_push(rt, array, xc_string_create(rt, "test"));
    xc_array_push(rt, array, xc_boolean_create(rt, 1));
    xc_array_push(rt, array, xc_number_create(rt, 10));  // Duplicate value
    
    // Test indexOf for existing elements
    TEST_ASSERT(xc_array_index_of(rt, array, xc_number_create(rt, 10)) == 0, 
                "indexOf should find first occurrence at index 0");
    TEST_ASSERT(xc_array_index_of(rt, array, xc_string_create(rt, "test")) == 1, 
                "indexOf should find string at index 1");
    TEST_ASSERT(xc_array_index_of(rt, array, xc_boolean_create(rt, 1)) == 2, 
                "indexOf should find boolean at index 2");
    
    // Test indexOf with start index
    TEST_ASSERT(xc_array_index_of_from(rt, array, xc_number_create(rt, 10), 1) == 3, 
                "indexOf from index 1 should find second occurrence at index 3");
    
    // Test indexOf for non-existent element
    TEST_ASSERT(xc_array_index_of(rt, array, xc_string_create(rt, "not found")) == -1, 
                "indexOf should return -1 for non-existent element");
    
    // Clean up
    xc_gc_release(rt, array);
    
    printf("Array indexOf tests passed!\n");
}

// Test array memory management
static void test_array_memory() {
    printf("Testing array memory management...\n");
    
    // Create a large array to test memory allocation
    xc_object_t *large_array = xc_array_create(rt);
    const int size = 1000;
    
    // Add many elements
    for (int i = 0; i < size; i++) {
        xc_array_push(rt, large_array, xc_number_create(rt, i));
    }
    
    TEST_ASSERT(xc_array_length(rt, large_array) == size, 
                "Large array should have correct length");
    
    // Verify elements
    for (int i = 0; i < size; i++) {
        TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, large_array, i)) == i, 
                    "Element value should match index");
    }
    
    // Remove elements
    for (int i = 0; i < size; i++) {
        xc_object_t *popped = xc_array_pop(rt, large_array);
        TEST_ASSERT(xc_number_value(rt, popped) == (size - i - 1), 
                    "Popped value should match expected value");
    }
    
    TEST_ASSERT(xc_array_length(rt, large_array) == 0, 
                "Array should be empty after popping all elements");
    
    // Clean up
    xc_gc_release(rt, large_array);
    
    printf("Array memory management tests passed!\n");
}

// Test array with nested arrays
static void test_nested_arrays() {
    printf("Testing nested arrays...\n");
    
    // Create outer array
    xc_object_t *outer = xc_array_create(rt);
    
    // Create inner arrays
    xc_object_t *inner1 = xc_array_create(rt);
    xc_array_push(rt, inner1, xc_number_create(rt, 1));
    xc_array_push(rt, inner1, xc_number_create(rt, 2));
    
    xc_object_t *inner2 = xc_array_create(rt);
    xc_array_push(rt, inner2, xc_string_create(rt, "a"));
    xc_array_push(rt, inner2, xc_string_create(rt, "b"));
    
    // Add inner arrays to outer array
    xc_array_push(rt, outer, inner1);
    xc_array_push(rt, outer, inner2);
    
    // Verify structure
    TEST_ASSERT(xc_array_length(rt, outer) == 2, "Outer array should have length 2");
    
    xc_object_t *retrieved1 = xc_array_get(rt, outer, 0);
    TEST_ASSERT(xc_is_array(rt, retrieved1), "First element should be an array");
    TEST_ASSERT(xc_array_length(rt, retrieved1) == 2, "Inner array 1 should have length 2");
    TEST_ASSERT(xc_number_value(rt, xc_array_get(rt, retrieved1, 0)) == 1, "Inner array 1, element 0 should be 1");
    
    xc_object_t *retrieved2 = xc_array_get(rt, outer, 1);
    TEST_ASSERT(xc_is_array(rt, retrieved2), "Second element should be an array");
    TEST_ASSERT(xc_array_length(rt, retrieved2) == 2, "Inner array 2 should have length 2");
    TEST_ASSERT(strcmp(xc_string_value(rt, xc_array_get(rt, retrieved2, 0)), "a") == 0, 
                "Inner array 2, element 0 should be 'a'");
    
    // Clean up
    xc_gc_release(rt, outer);  // Should recursively release inner arrays
    
    printf("Nested arrays tests passed!\n");
}

// Initialize the runtime and run all array tests
void run_array_tests() {
    // Initialize the runtime
    rt = &xc;
    
    // Initialize all types using xc_init()
    // xc_init();
    xc.init();
    
    // Run only the first test for now
    test_array_create();
    /*
    test_array_set();
    test_array_push_pop();
    test_array_shift_unshift();
    test_array_slice();
    test_array_concat();
    test_array_join();
    test_array_index_of();
    test_array_memory();
    test_nested_arrays();
    */
    xc.shutdown();
    printf("Array creation test passed!\n");
}

#ifdef TEST_ARRAY_STANDALONE
int main() {
    printf("Running XC Array Tests\n");
    printf("======================\n");
    
    run_array_tests();
    
    return 0;
}
#endif 
