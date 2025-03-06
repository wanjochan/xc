/*
 * xc_compat.c - XC兼容性层实现
 * 
 * 此文件实现了公共API (libxc.h) 和内部实现之间的兼容层
 * 解决类型和函数名称不匹配的问题
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xc.h"
#include "xc_object.h"
#include "xc_types/xc_array.h"
#include "xc_types/xc_types.h"
#include "../../include/xc_compat.h"

/* 初始化和清理函数 */
xc_runtime_t* xc_init(void) {
    return xc_runtime_init();
}

void xc_shutdown(void) {
    xc_runtime_shutdown();
}

/* 垃圾回收函数 */
void xc_gc_collect(void) {
    xc.gc();
}

xc_object_t* xc_gc_retain(xc_runtime_t *rt, xc_object_t *obj) {
    return (xc_object_t*)xc.retain((xc_val)obj);
}

void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj) {
    xc.release((xc_val)obj);
}

/* 类型检查函数 */
int xc_is_null(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_null_object(rt, obj);
}

int xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_boolean_object(rt, obj);
}

int xc_is_number(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_number_object(rt, obj);
}

int xc_is_string(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_string_object(rt, obj);
}

int xc_is_array(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_array_object(rt, obj);
}

int xc_is_object(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_generic_object(rt, obj);
}

int xc_is_function(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_function_object(rt, obj);
}

/* 值访问函数 */
int xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_boolean_get_value(rt, obj);
}

double xc_number_value(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_number_get_value(rt, obj);
}

const char* xc_string_value(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_string_get_value(rt, obj);
}

/* 创建函数 */
xc_object_t* xc_null_create(xc_runtime_t *rt) {
    return xc_null_object_create(rt);
}

xc_object_t* xc_boolean_create(xc_runtime_t *rt, int value) {
    return xc_boolean_object_create(rt, value);
}

xc_object_t* xc_number_create(xc_runtime_t *rt, double value) {
    return xc_number_object_create(rt, value);
}

xc_object_t* xc_string_create(xc_runtime_t *rt, const char *value) {
    return xc_string_object_create(rt, value);
}

xc_object_t* xc_array_create(xc_runtime_t *rt) {
    return xc_array_object_create(rt);
}

xc_object_t* xc_array_create_with_capacity(xc_runtime_t *rt, size_t initial_capacity) {
    return xc_array_object_create_with_capacity(rt, initial_capacity);
}

xc_object_t* xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count) {
    return xc_array_object_create_with_values(rt, values, count);
}

/* 数组操作函数 */
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *arr) {
    return xc_array_get_length(rt, arr);
}

xc_object_t* xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index) {
    return xc_array_get_element(rt, arr, index);
}

void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value) {
    xc_array_set_element(rt, arr, index, value);
}

void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    xc_array_push_element(rt, arr, value);
}

xc_object_t* xc_array_pop(xc_runtime_t *rt, xc_object_t *arr) {
    return xc_array_pop_element(rt, arr);
}

void xc_array_unshift(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    xc_array_unshift_element(rt, arr, value);
}

xc_object_t* xc_array_shift(xc_runtime_t *rt, xc_object_t *arr) {
    return xc_array_shift_element(rt, arr);
}

xc_object_t* xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end) {
    return xc_array_slice_elements(rt, arr, start, end);
}

xc_object_t* xc_array_concat(xc_runtime_t *rt, xc_object_t *arr1, xc_object_t *arr2) {
    return xc_array_concat_arrays(rt, arr1, arr2);
}

xc_object_t* xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator) {
    return xc_array_join_elements(rt, arr, separator);
}

int xc_array_index_of(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    return xc_array_find_index(rt, arr, value);
}

int xc_array_index_of_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index) {
    return xc_array_find_index_from(rt, arr, value, from_index);
} 