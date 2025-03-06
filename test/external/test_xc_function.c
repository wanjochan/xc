/**
 * External Test for XC Function Functionality
 * 
 * This test validates the function and closure functionality through the public API.
 * It only uses functions and types defined in libxc.h.
 */
#include "test_utils.h"
#include <stdio.h>
#include <string.h>

/* Test basic function functionality */
void test_function_basic(void) {
    test_start("Function Basic Functionality (External)");
    
    printf("Testing basic function functionality through public API...\n");
    
    // Initialize XC runtime
    xc.init();
    
    // Create a simple function that adds two numbers
    xc_val add_func = xc.create(XC_TYPE_FUNC, "function(a, b) { return a + b; }");
    TEST_ASSERT(add_func != NULL, "Function creation failed");
    TEST_ASSERT(xc.is(add_func, XC_TYPE_FUNC), "Function type check failed");
    
    // Create arguments
    xc_val arg1 = xc.create(XC_TYPE_NUMBER, 5.0);
    xc_val arg2 = xc.create(XC_TYPE_NUMBER, 7.0);
    
    // Invoke the function
    xc_val result = xc.invoke(add_func, 2, arg1, arg2);
    TEST_ASSERT(result != NULL, "Function invocation failed");
    TEST_ASSERT(xc.is(result, XC_TYPE_NUMBER), "Function result type check failed");
    
    // Clean up
    xc.gc();
    xc.shutdown();
    
    printf("Function test completed successfully.\n");
    
    test_end("Function Basic Functionality (External)");
}

/* Test function closure functionality */
void test_function_closure(void) {
    test_start("Function Closure Functionality (External)");
    
    printf("Testing function closure functionality through public API...\n");
    
    // Initialize XC runtime
    xc.init();
    
    // Create an object to hold our counter
    xc_val counter_obj = xc.create(XC_TYPE_OBJECT);
    xc_val initial_value = xc.create(XC_TYPE_NUMBER, 0.0);
    xc.dot(counter_obj, "count", initial_value);
    
    // Create a function that increments the counter and returns the new value
    xc_val increment_func = xc.create(XC_TYPE_FUNC, "function() { this.count += 1; return this.count; }");
    
    // Bind the function to the counter object
    xc.call(counter_obj, "bindMethod", "increment", increment_func);
    
    // Get the bound method
    xc_val bound_method = xc.dot(counter_obj, "increment");
    TEST_ASSERT(bound_method != NULL, "Method binding failed");
    
    // Call the method multiple times
    xc_val result1 = xc.invoke(bound_method, 0);
    TEST_ASSERT(result1 != NULL, "First method invocation failed");
    
    xc_val result2 = xc.invoke(bound_method, 0);
    TEST_ASSERT(result2 != NULL, "Second method invocation failed");
    
    xc_val result3 = xc.invoke(bound_method, 0);
    TEST_ASSERT(result3 != NULL, "Third method invocation failed");
    
    // Check the final counter value
    xc_val final_count = xc.dot(counter_obj, "count");
    TEST_ASSERT(final_count != NULL, "Counter retrieval failed");
    
    // Clean up
    xc.gc();
    xc.shutdown();
    
    printf("Function closure test completed successfully.\n");
    
    test_end("Function Closure Functionality (External)");
} 