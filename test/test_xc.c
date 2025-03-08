/*
 * test_rt->c - XC Runtime Tests Main Entry
 */

 DELETE OLD FILE?

#include "test_utils.h"

static xc_runtime_t* rt = NULL;

/* External test suite declarations */
void test_xc_types(void);
void test_xc_gc(void);
void test_xc_exception(void);
void register_composite_type_tests(void);
void register_object_tests(void);
void register_stdc_tests(void);
void run_array_tests(void); /* New array tests */

/* Simple print function for debugging */
static void print_val(xc_val val) {
    if (!val) {
        printf("null");
        return;
    }
    int type = rt->type_of(val);
    printf("Object(type=%d)", type);
}

/* Test runtime interface availability */
static void test_runtime_interface(void) {
    test_start("Runtime Interface");
    
    TEST_ASSERT_NOT_NULL(rt->alloc_object, "alloc_object interface available");
    TEST_ASSERT_NOT_NULL(rt->type_of, "type_of interface available");
    TEST_ASSERT_NOT_NULL(rt->create, "create interface available");
    TEST_ASSERT_NOT_NULL(rt->is, "is interface available");
    TEST_ASSERT_NOT_NULL(rt->call, "call interface available");
    //TEST_ASSERT_NOT_NULL(rt->gc, "gc interface available");
    TEST_ASSERT_NOT_NULL(rt->try_catch_finally, "exception handling available");
    
    test_end("Runtime Interface");
}

/* Register all test suites */
static void register_test_suites(void) {
    test_register("runtime.interface", test_runtime_interface, "core", 
                 "Test runtime interface availability");
}

int main(int argc, char* argv[]) {
    rt = xc_singleton();
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
    
    /* Run array tests */
    // run_array_tests();  // 暂时注释掉，需要重写数组测试以使用公共API
    
    /* Run all registered tests */
    test_run_all();
    
    /* Show test summary */
    test_summary();
    
    return g_test_suite.failures > 0 ? 1 : 0;
}
