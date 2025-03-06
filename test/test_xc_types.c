/*
 * test_xc_types.c - XC Type System Tests
 */

#include "test_utils.h"
#include "xc.h"

/* Boolean type tests */
static void test_boolean_creation(void) {
    test_start("Boolean Creation");
    
    /* Test creating boolean values */
    xc_val true_val = xc.create(XC_TYPE_BOOL, XC_TRUE);
    xc_val false_val = xc.create(XC_TYPE_BOOL, XC_FALSE);
    
    /* Test type checking */
    TEST_ASSERT_NOT_NULL(true_val, "Create TRUE value");
    TEST_ASSERT_NOT_NULL(false_val, "Create FALSE value");
    
    TEST_ASSERT_TYPE(true_val, XC_TYPE_BOOL, "TRUE type check");
    TEST_ASSERT_TYPE(false_val, XC_TYPE_BOOL, "FALSE type check");
    
    /* Test wrong type check */
    TEST_ASSERT(!xc.is(true_val, XC_TYPE_NUMBER), "Boolean is not number");
    
    test_end("Boolean Creation");
}

static void test_boolean_operations(void) {
    test_start("Boolean Operations");
    
    /* Create test values */
    xc_val true_val = xc.create(XC_TYPE_BOOL, XC_TRUE);
    xc_val false_val = xc.create(XC_TYPE_BOOL, XC_FALSE);
    
    /* Test NOT operation */
    xc_val not_result = xc.call(true_val, "not");
    TEST_ASSERT_NOT_NULL(not_result, "NOT operation result");
    TEST_ASSERT_TYPE(not_result, XC_TYPE_BOOL, "NOT operation type");
    
    /* Test AND operation */
    xc_val and_result = xc.call(true_val, "and", false_val);
    TEST_ASSERT_NOT_NULL(and_result, "AND operation result");
    TEST_ASSERT_TYPE(and_result, XC_TYPE_BOOL, "AND operation type");
    
    /* Test OR operation */
    xc_val or_result = xc.call(true_val, "or", false_val);
    TEST_ASSERT_NOT_NULL(or_result, "OR operation result");
    TEST_ASSERT_TYPE(or_result, XC_TYPE_BOOL, "OR operation type");
    
    /* Test XOR operation */
    xc_val xor_result = xc.call(true_val, "xor", false_val);
    TEST_ASSERT_NOT_NULL(xor_result, "XOR operation result");
    TEST_ASSERT_TYPE(xor_result, XC_TYPE_BOOL, "XOR operation type");
    
    test_end("Boolean Operations");
}

static void test_boolean_conversion(void) {
    test_start("Boolean Conversion");
    
    /* Create test values */
    xc_val true_val = xc.create(XC_TYPE_BOOL, XC_TRUE);
    xc_val false_val = xc.create(XC_TYPE_BOOL, XC_FALSE);
    
    /* Test toString conversion */
    xc_val true_str = xc.call(true_val, "toString");
    xc_val false_str = xc.call(false_val, "toString");
    
    TEST_ASSERT_NOT_NULL(true_str, "TRUE toString result");
    TEST_ASSERT_NOT_NULL(false_str, "FALSE toString result");
    TEST_ASSERT_TYPE(true_str, XC_TYPE_STRING, "TRUE toString type");
    TEST_ASSERT_TYPE(false_str, XC_TYPE_STRING, "FALSE toString type");
    
    test_end("Boolean Conversion");
}

/* Register boolean tests */
void register_boolean_tests(void) {
    test_register("boolean.creation", test_boolean_creation, "types", 
                 "Test boolean value creation and type checking");
    test_register("boolean.operations", test_boolean_operations, "types",
                 "Test boolean logical operations");
    test_register("boolean.conversion", test_boolean_conversion, "types",
                 "Test boolean type conversion");
}

/* Forward declarations */
void register_composite_type_tests(void);

/* Test runner for basic types */
void test_xc_types(void) {
    test_init("XC Basic Types Test Suite");
    
    /* Register all type tests */
    register_boolean_tests();
    /* Composite type tests are registered in main */
    
    /* Don't run tests here, they will be run in main */
    
    /* Clean up */
    test_cleanup();
}