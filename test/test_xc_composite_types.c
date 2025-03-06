/*
 * test_xc_composite_types.c - XC Composite Types Tests
 */

#include "test_utils.h"
#include "xc.h"
#include <stdio.h>
#include <string.h>

/* Test helper functions */
static xc_val forEach_test_func(xc_val self, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !closure) return NULL;
    
    /* Get the sum accumulator from closure */
    xc_val sum = closure;
    if (!xc.is(sum, XC_TYPE_NUMBER)) return NULL;
    
    /* Add current value to sum */
    if (xc.is(argv[0], XC_TYPE_NUMBER)) {
        double current = *(double*)sum;
        current += *(double*)argv[0];
        /* Update the sum value directly */
        *(double*)sum = current;
    }
    
    return NULL;
}

static xc_val map_test_func(xc_val self, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0]) return NULL;
    
    /* Double the input value */
    if (xc.is(argv[0], XC_TYPE_NUMBER)) {
        double value = *(double*)argv[0];
        return xc.create(XC_TYPE_NUMBER, value * 2);
    }
    
    return NULL;
}

static xc_val filter_test_func(xc_val self, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0]) return NULL;
    
    /* Keep only values > 1 */
    if (xc.is(argv[0], XC_TYPE_NUMBER)) {
        double value = *(double*)argv[0];
        return xc.create(XC_TYPE_BOOL, value > 1 ? XC_TRUE : XC_FALSE);
    }
    
    return NULL;
}

/* Array type tests */
static void test_array_creation(void) {
    test_start("Array Creation");
    
    /* Test creating empty array */
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    TEST_ASSERT_NOT_NULL(arr, "Create empty array");
    TEST_ASSERT_TYPE(arr, XC_TYPE_ARRAY, "Array type check");
    
    /* Test initial length */
    xc_val length = xc.call(arr, "length");
    TEST_ASSERT_NOT_NULL(length, "Length method call");
    TEST_ASSERT_TYPE(length, XC_TYPE_NUMBER, "Length return type");
    TEST_ASSERT(*(double*)length == 0, "Empty array length");
    
    /* Clean up */
    xc.release(arr);
    xc.release(length);
    
    test_end("Array Creation");
}

static void test_array_push_pop(void) {
    test_start("Array Push/Pop");
    
    /* Create test array */
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    
    /* Test push */
    xc_val num1 = xc.create(XC_TYPE_NUMBER, 42.0);
    xc_val push_result = xc.call(arr, "push", num1);
    TEST_ASSERT_NOT_NULL(push_result, "Push return value");
    
    /* Verify length */
    xc_val length = xc.call(arr, "length");
    TEST_ASSERT(*(double*)length == 1, "Array length after push");
    
    /* Test pop */
    xc_val pop_result = xc.call(arr, "pop");
    TEST_ASSERT_NOT_NULL(pop_result, "Pop return value");
    TEST_ASSERT_TYPE(pop_result, XC_TYPE_NUMBER, "Popped value type");
    TEST_ASSERT(*(double*)pop_result == 42.0, "Popped value");
    
    /* Verify empty */
    length = xc.call(arr, "length");
    TEST_ASSERT(*(double*)length == 0, "Array length after pop");
    
    /* Clean up */
    xc.release(arr);
    xc.release(num1);
    xc.release(push_result);
    xc.release(pop_result);
    xc.release(length);
    
    test_end("Array Push/Pop");
}

static void test_array_get_set(void) {
    test_start("Array Get/Set");
    
    /* Create test array */
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    
    /* Test set */
    xc_val num1 = xc.create(XC_TYPE_NUMBER, 42.0);
    xc_val index = xc.create(XC_TYPE_NUMBER, 0.0);
    xc_val set_result = xc.call(arr, "set", index, num1);
    TEST_ASSERT_NOT_NULL(set_result, "Set return value");
    
    /* Test get */
    xc_val get_result = xc.call(arr, "get", index);
    TEST_ASSERT_NOT_NULL(get_result, "Get return value");
    TEST_ASSERT_TYPE(get_result, XC_TYPE_NUMBER, "Get value type");
    TEST_ASSERT(*(double*)get_result == 42.0, "Get value");
    
    /* Test invalid index */
    xc_val invalid_index = xc.create(XC_TYPE_NUMBER, -1.0);
    get_result = xc.call(arr, "get", invalid_index);
    TEST_ASSERT(get_result == NULL, "Get invalid index");
    
    /* Clean up */
    xc.release(arr);
    xc.release(num1);
    xc.release(index);
    xc.release(invalid_index);
    xc.release(set_result);
    
    test_end("Array Get/Set");
}

static void test_array_methods(void) {
    test_start("Array Methods");
    
    /* Create test array */
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    
    /* Add test elements */
    xc_val num1 = xc.create(XC_TYPE_NUMBER, 1.0);
    xc_val num2 = xc.create(XC_TYPE_NUMBER, 2.0);
    xc.call(arr, "push", num1);
    xc.call(arr, "push", num2);
    
    /* Test join */
    xc_val comma = xc.create(XC_TYPE_STRING, ",");
    xc_val join_result = xc.call(arr, "join", comma);
    TEST_ASSERT_NOT_NULL(join_result, "Join return value");
    TEST_ASSERT_TYPE(join_result, XC_TYPE_STRING, "Join return type");
    
    /* Test toString */
    xc_val str_result = xc.call(arr, "toString");
    TEST_ASSERT_NOT_NULL(str_result, "ToString return value");
    TEST_ASSERT_TYPE(str_result, XC_TYPE_STRING, "ToString return type");
    
    /* Clean up */
    xc.release(num1);
    xc.release(num2);
    xc.release(arr);
    xc.release(comma);
    xc.release(join_result);
    xc.release(str_result);
    
    test_end("Array Methods");
}

/* Test array high level functions */
static void test_array_high_level(void) {
    test_start("Array High Level Functions");
    
    /* Create test array */
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    
    /* Add test elements */
    for (int i = 1; i <= 3; i++) {
        xc_val num = xc.create(XC_TYPE_NUMBER, (double)i);
        xc.call(arr, "push", num);
        xc.release(num);
    }
    
    /* Test forEach */
    xc_val sum = xc.create(XC_TYPE_NUMBER, 0.0);
    xc_val forEach_cb = xc.create(XC_TYPE_FUNC, forEach_test_func, sum);
    xc_val result = xc.call(arr, "forEach", forEach_cb);
    TEST_ASSERT_NOT_NULL(result, "ForEach return value");
    TEST_ASSERT(*(double*)sum == 6.0, "ForEach sum calculation");
    
    /* Test map */
    xc_val map_cb = xc.create(XC_TYPE_FUNC, map_test_func, NULL);
    xc_val mapped = xc.call(arr, "map", map_cb);
    TEST_ASSERT_NOT_NULL(mapped, "Map return value");
    TEST_ASSERT_TYPE(mapped, XC_TYPE_ARRAY, "Map return type");
    
    xc_val length = xc.call(mapped, "length");
    TEST_ASSERT(*(double*)length == 3.0, "Mapped array length");
    
    /* Test filter */
    xc_val filter_cb = xc.create(XC_TYPE_FUNC, filter_test_func, NULL);
    xc_val filtered = xc.call(arr, "filter", filter_cb);
    TEST_ASSERT_NOT_NULL(filtered, "Filter return value");
    TEST_ASSERT_TYPE(filtered, XC_TYPE_ARRAY, "Filter return type");
    
    length = xc.call(filtered, "length");
    TEST_ASSERT(*(double*)length == 2.0, "Filtered array length");
    
    /* Clean up */
    xc.release(arr);
    xc.release(forEach_cb);
    xc.release(map_cb);
    xc.release(filter_cb);
    xc.release(mapped);
    xc.release(filtered);
    xc.release(sum);
    xc.release(result);
    xc.release(length);
    
    test_end("Array High Level Functions");
}

/* Object type tests */
static void test_object_creation(void) {
    test_start("Object Creation");
    
    /* Test creating empty object */
    xc_val obj = xc.create(XC_TYPE_OBJECT, 0);
    TEST_ASSERT_NOT_NULL(obj, "Create empty object");
    TEST_ASSERT_TYPE(obj, XC_TYPE_OBJECT, "Object type check");
    
    /* Test keys of empty object */
    xc_val keys = xc.call(obj, "keys");
    TEST_ASSERT_NOT_NULL(keys, "Keys method call");
    TEST_ASSERT_TYPE(keys, XC_TYPE_ARRAY, "Keys return type");
    
    xc_val length = xc.call(keys, "length");
    TEST_ASSERT(*(double*)length == 0, "Empty object keys length");
    
    /* Clean up */
    xc.release(obj);
    xc.release(keys);
    xc.release(length);
    
    test_end("Object Creation");
}

static void test_object_get_set(void) {
    test_start("Object Get/Set");
    
    /* Create test object */
    xc_val obj = xc.create(XC_TYPE_OBJECT, 0);
    
    /* Test set */
    xc_val key = xc.create(XC_TYPE_STRING, "test");
    xc_val value = xc.create(XC_TYPE_NUMBER, 42.0);
    xc_val set_result = xc.call(obj, "set", key, value);
    TEST_ASSERT_NOT_NULL(set_result, "Set return value");
    
    /* Test get */
    xc_val get_result = xc.call(obj, "get", key);
    TEST_ASSERT_NOT_NULL(get_result, "Get return value");
    TEST_ASSERT_TYPE(get_result, XC_TYPE_NUMBER, "Get value type");
    TEST_ASSERT(*(double*)get_result == 42.0, "Get value");
    
    /* Test invalid key */
    xc_val invalid_key = xc.create(XC_TYPE_STRING, "nonexistent");
    get_result = xc.call(obj, "get", invalid_key);
    TEST_ASSERT(get_result == NULL, "Get invalid key");
    
    /* Clean up */
    xc.release(obj);
    xc.release(key);
    xc.release(value);
    xc.release(set_result);
    xc.release(invalid_key);
    
    test_end("Object Get/Set");
}

static void test_object_methods(void) {
    test_start("Object Methods");
    
    /* Create test object */
    xc_val obj = xc.create(XC_TYPE_OBJECT, 0);
    
    /* Add test properties */
    xc_val key1 = xc.create(XC_TYPE_STRING, "a");
    xc_val key2 = xc.create(XC_TYPE_STRING, "b");
    xc_val val1 = xc.create(XC_TYPE_NUMBER, 1.0);
    xc_val val2 = xc.create(XC_TYPE_NUMBER, 2.0);
    xc.call(obj, "set", key1, val1);
    xc.call(obj, "set", key2, val2);
    
    /* Test has */
    xc_val has_result = xc.call(obj, "has", key1);
    TEST_ASSERT_NOT_NULL(has_result, "Has return value");
    TEST_ASSERT_TYPE(has_result, XC_TYPE_BOOL, "Has return type");
    TEST_ASSERT(*(bool*)has_result == true, "Has existing key");
    
    /* Test keys */
    xc_val keys = xc.call(obj, "keys");
    TEST_ASSERT_NOT_NULL(keys, "Keys return value");
    TEST_ASSERT_TYPE(keys, XC_TYPE_ARRAY, "Keys return type");
    
    xc_val length = xc.call(keys, "length");
    TEST_ASSERT(*(double*)length == 2, "Object keys length");
    
    /* Test values */
    xc_val values = xc.call(obj, "values");
    TEST_ASSERT_NOT_NULL(values, "Values return value");
    TEST_ASSERT_TYPE(values, XC_TYPE_ARRAY, "Values return type");
    
    length = xc.call(values, "length");
    TEST_ASSERT(*(double*)length == 2, "Object values length");
    
    /* Test entries */
    xc_val entries = xc.call(obj, "entries");
    TEST_ASSERT_NOT_NULL(entries, "Entries return value");
    TEST_ASSERT_TYPE(entries, XC_TYPE_ARRAY, "Entries return type");
    
    length = xc.call(entries, "length");
    TEST_ASSERT(*(double*)length == 2, "Object entries length");
    
    /* Test toString */
    xc_val str_result = xc.call(obj, "toString");
    TEST_ASSERT_NOT_NULL(str_result, "ToString return value");
    TEST_ASSERT_TYPE(str_result, XC_TYPE_STRING, "ToString return type");
    
    /* Clean up */
    xc.release(obj);
    xc.release(key1);
    xc.release(key2);
    xc.release(val1);
    xc.release(val2);
    xc.release(has_result);
    xc.release(keys);
    xc.release(values);
    xc.release(entries);
    xc.release(str_result);
    xc.release(length);
    
    test_end("Object Methods");
}

static void test_object_delete(void) {
    test_start("Object Delete");
    
    /* Create test object */
    xc_val obj = xc.create(XC_TYPE_OBJECT, 0);
    
    /* Add test property */
    xc_val key = xc.create(XC_TYPE_STRING, "test");
    xc_val value = xc.create(XC_TYPE_NUMBER, 42.0);
    xc.call(obj, "set", key, value);
    
    /* Test delete */
    xc_val delete_result = xc.call(obj, "delete", key);
    TEST_ASSERT_NOT_NULL(delete_result, "Delete return value");
    TEST_ASSERT_TYPE(delete_result, XC_TYPE_BOOL, "Delete return type");
    TEST_ASSERT(*(bool*)delete_result == true, "Delete success");
    
    /* Verify property is gone */
    xc_val has_result = xc.call(obj, "has", key);
    TEST_ASSERT(*(bool*)has_result == false, "Property deleted");
    
    /* Clean up */
    xc.release(obj);
    xc.release(key);
    xc.release(value);
    xc.release(delete_result);
    xc.release(has_result);
    
    test_end("Object Delete");
}

/* Complex tests */
static void test_complex_structures(void) {
    test_start("Complex Structures");
    
    /* Create nested structure */
    xc_val obj = xc.create(XC_TYPE_OBJECT, 0);
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    
    /* Add array to object */
    xc_val key = xc.create(XC_TYPE_STRING, "array");
    xc.call(obj, "set", key, arr);
    
    /* Add object to array */
    xc_val inner_obj = xc.create(XC_TYPE_OBJECT, 0);
    xc.call(arr, "push", inner_obj);
    
    /* Verify structure */
    xc_val get_result = xc.call(obj, "get", key);
    TEST_ASSERT_TYPE(get_result, XC_TYPE_ARRAY, "Get array from object");
    
    xc_val index = xc.create(XC_TYPE_NUMBER, 0.0);
    xc_val get_result2 = xc.call(get_result, "get", index);
    TEST_ASSERT_TYPE(get_result2, XC_TYPE_OBJECT, "Get object from array");
    
    /* Clean up */
    xc.release(obj);
    xc.release(arr);
    xc.release(key);
    xc.release(inner_obj);
    xc.release(index);
    xc.release(get_result);
    xc.release(get_result2);
    
    test_end("Complex Structures");
}

/* Register all composite type tests */
void register_composite_type_tests(void) {
    /* Array tests */
    test_register("array.creation", test_array_creation, "types",
                 "Test array creation and type checking");
    test_register("array.push_pop", test_array_push_pop, "types",
                 "Test array push and pop operations");
    test_register("array.get_set", test_array_get_set, "types",
                 "Test array get and set operations");
    test_register("array.methods", test_array_methods, "types",
                 "Test array utility methods");
    test_register("array.high_level", test_array_high_level, "types",
                 "Test array high level functions");
    
    /* Object tests */
    test_register("object.creation", test_object_creation, "types",
                 "Test object creation and type checking");
    test_register("object.get_set", test_object_get_set, "types",
                 "Test object get and set operations");
    test_register("object.methods", test_object_methods, "types",
                 "Test object utility methods");
    test_register("object.delete", test_object_delete, "types",
                 "Test object delete operation");
    
    /* Complex tests */
    test_register("complex.structures", test_complex_structures, "types",
                 "Test complex nested structures");
}