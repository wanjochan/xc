#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global test suite */
xc_test_suite g_test_suite = {0};

/* Initialize the test suite */
void test_init(const char* suite_name) {
    g_test_suite.name = suite_name;
    g_test_suite.cases = NULL;
    g_test_suite.case_count = 0;
    g_test_suite.failures = 0;
    g_test_suite.total_assertions = 0;
    g_test_suite.failed_assertions = 0;
    
    /* Print header */
    printf("\n===== Starting Test Suite: %s =====\n\n", suite_name);
}

/* Clean up the test suite */
void test_cleanup(void) {
    if (g_test_suite.cases) {
        free(g_test_suite.cases);
        g_test_suite.cases = NULL;
    }
    g_test_suite.case_count = 0;
}

/* Start a test case */
void test_start(const char* name) {
    printf("\n----- Starting Test: %s -----\n\n", name);
}

/* End a test case */
void test_end(const char* name) {
    printf("\n----- Completed Test: %s -----\n", name);
}

/* Register a test case */
void test_register(const char* name, void (*func)(void), const char* category, const char* description) {
    g_test_suite.case_count++;
    g_test_suite.cases = realloc(g_test_suite.cases, g_test_suite.case_count * sizeof(xc_test_case));
    
    g_test_suite.cases[g_test_suite.case_count - 1].name = name;
    g_test_suite.cases[g_test_suite.case_count - 1].func = func;
    g_test_suite.cases[g_test_suite.case_count - 1].category = category;
    g_test_suite.cases[g_test_suite.case_count - 1].description = description;
}

/* Run all test cases */
void test_run_all(void) {
    int prev_failures = g_test_suite.failures;
    int prev_assertions = g_test_suite.total_assertions;
    int prev_failed_assertions = g_test_suite.failed_assertions;
    
    printf("\n===== Running All Tests =====\n\n");
    
    for (int i = 0; i < g_test_suite.case_count; i++) {
        xc_test_case* test = &g_test_suite.cases[i];
        
        printf("Running Test %d/%d: %s\n", i+1, g_test_suite.case_count, test->name);
        printf("Category: %s\n", test->category);
        printf("Description: %s\n\n", test->description);
        
        test_start(test->name);
        test->func();
        test_end(test->name);
        
        int curr_failures = g_test_suite.failures - prev_failures;
        int curr_assertions = g_test_suite.total_assertions - prev_assertions;
        int curr_failed_assertions = g_test_suite.failed_assertions - prev_failed_assertions;
        
        printf("\nTest Results:\n");
        printf("- Assertions: %d\n", curr_assertions);
        printf("- Failures: %d\n", curr_failed_assertions);
        printf("- Status: %s\n\n", curr_failures > 0 ? "FAILED" : "PASSED");
        
        prev_failures = g_test_suite.failures;
        prev_assertions = g_test_suite.total_assertions;
        prev_failed_assertions = g_test_suite.failed_assertions;
    }
    
    test_summary();
}

/* Run tests in a specific category */
void test_run_category(const char* category) {
    int prev_failures = g_test_suite.failures;
    int prev_assertions = g_test_suite.total_assertions;
    int prev_failed_assertions = g_test_suite.failed_assertions;
    int tests_run = 0;
    
    printf("\n===== Running Tests in Category: %s =====\n\n", category);
    
    for (int i = 0; i < g_test_suite.case_count; i++) {
        xc_test_case* test = &g_test_suite.cases[i];
        
        if (strcmp(test->category, category) == 0) {
            tests_run++;
            
            printf("Running Test %d: %s\n", tests_run, test->name);
            printf("Description: %s\n\n", test->description);
            
            test_start(test->name);
            test->func();
            test_end(test->name);
            
            int curr_failures = g_test_suite.failures - prev_failures;
            int curr_assertions = g_test_suite.total_assertions - prev_assertions;
            int curr_failed_assertions = g_test_suite.failed_assertions - prev_failed_assertions;
            
            printf("\nTest Results:\n");
            printf("- Assertions: %d\n", curr_assertions);
            printf("- Failures: %d\n", curr_failed_assertions);
            printf("- Status: %s\n\n", curr_failures > 0 ? "FAILED" : "PASSED");
            
            prev_failures = g_test_suite.failures;
            prev_assertions = g_test_suite.total_assertions;
            prev_failed_assertions = g_test_suite.failed_assertions;
        }
    }
    
    if (tests_run == 0) {
        printf("No tests found in category '%s'\n", category);
    }
}

/* Print a summary of the test results */
void test_summary(void) {
    double success_rate = 100.0;
    if (g_test_suite.total_assertions > 0) {
        success_rate = 100.0 * (g_test_suite.total_assertions - g_test_suite.failed_assertions) / g_test_suite.total_assertions;
    }
    
    printf("\n===== Test Summary =====\n");
    printf("Suite: %s\n", g_test_suite.name);
    printf("Total Test Cases: %d\n", g_test_suite.case_count);
    printf("Failed Test Cases: %d\n", g_test_suite.failures);
    printf("Total Assertions: %d\n", g_test_suite.total_assertions);
    printf("Failed Assertions: %d\n", g_test_suite.failed_assertions);
    printf("Success Rate: %.1f%%\n", success_rate);
    printf("=======================\n\n");
} 