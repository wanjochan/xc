/*
 * test_xc.c - XC Runtime Tests Main Entry
 */

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
    
    //TEST_ASSERT_NOT_NULL(xc.alloc_object, "alloc_object interface available");
    TEST_ASSERT_NOT_NULL(rt->type_of, "type_of interface available");
    TEST_ASSERT_NOT_NULL(rt->new, "new interface available");
    TEST_ASSERT_NOT_NULL(rt->is, "is interface available");
    TEST_ASSERT_NOT_NULL(rt->call, "call interface available");
    // TEST_ASSERT_NOT_NULL(xc.gc, "gc interface available"); // gc is not in the xc_runtime_t struct
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
    
    /* 
     * 按照从浅到深的顺序执行测试:
     * 1. 运行时接口测试
     * 2. 基础类型测试 (null, boolean, number, string)
     * 3. 异常处理测试
     * 4. 对象测试
     * 5. 数组测试
     */
    
    /* 1. 运行时接口测试 */
    printf("\n===== 1. 运行时接口测试 =====\n");
    test_run_category("core");
    
    /* 2. 基础类型测试 */
    printf("\n===== 2. 基础类型测试 =====\n");
    test_xc_types();
    
    /* 3. 异常处理测试 */
    printf("\n===== 3. 异常处理测试 =====\n");
    test_xc_exception();
    
    /* 4. 对象测试 */
    printf("\n===== 4. 对象测试 =====\n");
    register_object_tests();
    test_run_category("object");
    
    /* 5. 数组测试 */
    printf("\n===== 5. 数组测试 =====\n");
    run_array_tests();
    
    /* 其他测试暂时注释掉 */
    // test_xc_gc(); // 垃圾回收测试
    // register_composite_type_tests(); // 复合类型测试
    // register_stdc_tests(); // 标准库测试
    
    /* Show test summary */
    test_summary();
    
    return g_test_suite.failures > 0 ? 1 : 0;
}
