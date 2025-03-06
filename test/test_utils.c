/*
 * test_utils.c - XC Test Framework Core Utilities Implementation
 */

#include "test_utils.h"

/* Maximum number of test cases */
#define MAX_TEST_CASES 1000

/* Global test suite */
xc_test_suite g_test_suite = {
    .name = NULL,
    .cases = NULL,
    .case_count = 0,
    .failures = 0,
    .total_assertions = 0,
    .failed_assertions = 0
};

/* Static storage for test cases */
static xc_test_case test_cases[MAX_TEST_CASES];

/* Initialize test framework */
void test_init(const char* suite_name) {
    g_test_suite.name = suite_name;
    g_test_suite.cases = test_cases;
    g_test_suite.case_count = 0;
    g_test_suite.failures = 0;
    g_test_suite.total_assertions = 0;
    g_test_suite.failed_assertions = 0;
    
    printf("\n===== Starting Test Suite: %s =====\n\n", suite_name);
}

/* Clean up test framework */
void test_cleanup(void) {
    /* Currently nothing to clean up */
}

/* Start a test case */
void test_start(const char* name) {
    printf("\n----- Starting Test: %s -----\n", name);
}

/* End a test case */
void test_end(const char* name) {
    printf("----- Completed Test: %s -----\n\n", name);
}

/* Print test summary */
void test_summary(void) {
    printf("\n===== Test Summary =====\n");
    printf("Suite: %s\n", g_test_suite.name);
    printf("Total Test Cases: %d\n", g_test_suite.case_count);
    printf("Failed Test Cases: %d\n", g_test_suite.failures);
    printf("Total Assertions: %d\n", g_test_suite.total_assertions);
    printf("Failed Assertions: %d\n", g_test_suite.failed_assertions);
    printf("Success Rate: %.1f%%\n", 
           (g_test_suite.total_assertions - g_test_suite.failed_assertions) * 100.0f / 
           (g_test_suite.total_assertions > 0 ? g_test_suite.total_assertions : 1));
    printf("=======================\n\n");
}

/* Register a test case */
void test_register(const char* name, void (*func)(void), const char* category, const char* description) {
    if (g_test_suite.case_count >= MAX_TEST_CASES) {
        printf("ERROR: Cannot register test case '%s', maximum number of test cases reached\n", name);
        return;
    }
    
    xc_test_case* test_case = &g_test_suite.cases[g_test_suite.case_count++];
    test_case->name = name;
    test_case->func = func;
    test_case->category = category;
    test_case->description = description;
}

/* Run all registered test cases */
void test_run_all(void) {
    printf("\n===== Running All Tests =====\n\n");
    
    for (int i = 0; i < g_test_suite.case_count; i++) {
        xc_test_case* test_case = &g_test_suite.cases[i];
        
        printf("Running Test %d/%d: %s\n", 
               i + 1, g_test_suite.case_count, test_case->name);
        printf("Category: %s\n", test_case->category);
        printf("Description: %s\n", test_case->description);
        
        /* Record assertion counts before test */
        int prev_total = g_test_suite.total_assertions;
        int prev_failed = g_test_suite.failed_assertions;
        
        /* Run the test */
        test_start(test_case->name);
        test_case->func();
        test_end(test_case->name);
        
        /* Report test case results */
        int test_assertions = g_test_suite.total_assertions - prev_total;
        int test_failures = g_test_suite.failed_assertions - prev_failed;
        
        printf("Test Results:\n");
        printf("- Assertions: %d\n", test_assertions);
        printf("- Failures: %d\n", test_failures);
        printf("- Status: %s\n\n", test_failures == 0 ? "PASSED" : "FAILED");
    }
}

/* Run test cases in a specific category */
void test_run_category(const char* category) {
    printf("\n===== Running Tests in Category: %s =====\n\n", category);
    
    int count = 0;
    for (int i = 0; i < g_test_suite.case_count; i++) {
        xc_test_case* test_case = &g_test_suite.cases[i];
        
        if (strcmp(test_case->category, category) == 0) {
            count++;
            printf("Running Test %d: %s\n", count, test_case->name);
            printf("Description: %s\n", test_case->description);
            
            /* Record assertion counts before test */
            int prev_total = g_test_suite.total_assertions;
            int prev_failed = g_test_suite.failed_assertions;
            
            /* Run the test */
            test_start(test_case->name);
            test_case->func();
            test_end(test_case->name);
            
            /* Report test case results */
            int test_assertions = g_test_suite.total_assertions - prev_total;
            int test_failures = g_test_suite.failed_assertions - prev_failed;
            
            printf("Test Results:\n");
            printf("- Assertions: %d\n", test_assertions);
            printf("- Failures: %d\n", test_failures);
            printf("- Status: %s\n\n", test_failures == 0 ? "PASSED" : "FAILED");
        }
    }
    
    if (count == 0) {
        printf("No tests found in category: %s\n", category);
    }
}