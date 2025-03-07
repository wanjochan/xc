/*
 * test_utils.h - XC Internal Test Framework Core Utilities
 * 
 * This header provides the core testing utilities for the XC runtime internal tests (white-box tests),
 * including assertions, test case management, and result reporting.
 * 
 * NOTE: This is for INTERNAL testing using the internal API. For external testing using
 * the public API, see test/external/test_utils.h.
 */

#ifndef XC_INTERNAL_TEST_UTILS_H
#define XC_INTERNAL_TEST_UTILS_H

/* Include internal headers */
#include "../../src/xc/xc.h"
#include "../../src/xc/xc_internal.h"
// #include "../../src/xc/xc_gc.h"  // Removed since we've merged it into xc.c

/* Test case structure */
typedef struct xc_test_case {
    const char* name;           /* Test case name */
    void (*func)(void);        /* Test function */
    const char* category;      /* Test category (e.g., "types", "gc", etc.) */
    const char* description;   /* Test description */
} xc_test_case;

/* Test suite structure */
typedef struct xc_test_suite {
    const char* name;          /* Suite name */
    xc_test_case* cases;      /* Array of test cases */
    int case_count;           /* Number of test cases */
    int failures;             /* Number of failed tests */
    int total_assertions;     /* Total number of assertions */
    int failed_assertions;    /* Number of failed assertions */
} xc_test_suite;

/* Test assertion macros */
#define TEST_ASSERT(cond, msg) \
    do { \
        g_test_suite.total_assertions++; \
        if (cond) { \
            printf("✓ %s\n", msg); \
        } else { \
            printf("✗ %s\n", msg); \
            printf("  at %s:%d\n", __FILE__, __LINE__); \
            g_test_suite.failed_assertions++; \
            g_test_suite.failures++; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, msg) \
    TEST_ASSERT((expected) == (actual), msg ": expected " #expected " but got " #actual)

#define TEST_ASSERT_NOT_NULL(ptr, msg) \
    TEST_ASSERT((ptr) != NULL, msg ": expected non-null value")

#define TEST_ASSERT_TYPE(val, type, msg) \
    TEST_ASSERT(xc.is(val, type), msg ": expected type " #type)

/* Test lifecycle functions */
void test_init(const char* suite_name);
void test_cleanup(void);
void test_start(const char* name);
void test_end(const char* name);
void test_summary(void);

/* Test registration */
void test_register(const char* name, void (*func)(void), const char* category, const char* description);
void test_run_all(void);
void test_run_category(const char* category);

/* Global test suite */
extern xc_test_suite g_test_suite;

#endif /* XC_INTERNAL_TEST_UTILS_H */