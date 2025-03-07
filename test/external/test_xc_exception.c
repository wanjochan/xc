/**
 * External Test for XC Exception Handling
 * 
 * This test validates the exception handling functionality through the public API.
 * It only uses functions and types defined in libxc.h.
 */
#include "test_utils.h"
#include <stdio.h>
#include <string.h>

/* Global flag to track if catch handler was called */
static int catch_handler_called = 0;

/* Global flag to track if finally handler was called */
static int finally_handler_called = 0;

/* Function that throws an exception */
static xc_val throw_exception_func(xc_val self, xc_val args) {
    printf("Throwing exception...\n");
    
    // Create an error object
    xc_val error = xc.create(XC_TYPE_ERROR, "Test exception");
    
    // Throw the exception
    xc.throw(error);
    
    // This should not be reached
    printf("ERROR: Code after throw was executed!\n");
    return NULL;
}

/* Catch handler function */
static xc_val catch_handler(xc_val self, xc_val error) {
    printf("Catch handler called with error\n");
    catch_handler_called = 1;
    return NULL;
}

/* Finally handler function */
static xc_val finally_handler(xc_val self, xc_val args) {
    printf("Finally handler called\n");
    finally_handler_called = 1;
    return NULL;
}

/* Test basic exception handling */
void test_exception_basic(void) {
    test_start("Exception Basic Functionality (External)");
    
    printf("Testing basic exception handling through public API...\n");
    
    // Reset global flags
    catch_handler_called = 0;
    finally_handler_called = 0;
    
    // Create function objects
    xc_val throw_func = xc.create(XC_TYPE_FUNC, throw_exception_func);
    xc_val catch_func = xc.create(XC_TYPE_FUNC, catch_handler);
    xc_val finally_func = xc.create(XC_TYPE_FUNC, finally_handler);
    
    // Execute try-catch-finally
    xc.try_catch_finally(throw_func, catch_func, finally_func);
    
    // Verify handlers were called
    TEST_ASSERT(catch_handler_called == 1, "Catch handler was not called");
    TEST_ASSERT(finally_handler_called == 1, "Finally handler was not called");
    
    printf("Exception test completed successfully.\n");
    
    test_end("Exception Basic Functionality (External)");
}

/* Test uncaught exception handling */
void test_uncaught_exception(void) {
    test_start("Uncaught Exception Handling (External)");
    
    printf("Testing uncaught exception handling through public API...\n");
    
    // Create a custom uncaught exception handler
    xc_val uncaught_handler = xc.create(XC_TYPE_FUNC, "function(error) { return 'Uncaught: ' + error.message; }");
    
    // Set the uncaught exception handler
    xc.set_uncaught_exception_handler(uncaught_handler);
    
    // Create a function that throws an exception
    xc_val throw_func = xc.create(XC_TYPE_FUNC, throw_exception_func);
    
    // Execute try with no catch (should call uncaught handler)
    xc.try_catch_finally(throw_func, NULL, NULL);
    
    // Get the current error
    xc_val current_error = xc.get_current_error();
    TEST_ASSERT(current_error != NULL, "Current error should not be NULL");
    TEST_ASSERT(xc.is(current_error, XC_TYPE_ERROR), "Current error should be an error object");
    
    // Clear the error
    xc.clear_error();
    
    // Verify error was cleared
    current_error = xc.get_current_error();
    TEST_ASSERT(current_error == NULL, "Error should be cleared");
    
    printf("Uncaught exception test completed successfully.\n");
    
    test_end("Uncaught Exception Handling (External)");
} 
