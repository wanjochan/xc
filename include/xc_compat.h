/*
 * xc_compat.h - XC兼容性头文件
 * 
 * 此文件提供了公共API (libxc.h) 和内部实现之间的兼容层
 * 解决类型和函数名称不匹配的问题
 */
#ifndef XC_COMPAT_H
#define XC_COMPAT_H

/* 只包含公共API头文件，避免与内部头文件冲突 */
#include "libxc.h"

/* 前向声明内部类型 */
struct xc_object;
typedef struct xc_object xc_object_t;

struct xc_runtime;
typedef struct xc_runtime xc_runtime_t;

/* 类型兼容性定义 */
typedef xc_object_t* xc_value;  /* 将内部xc_object_t*映射到公共API的xc_val */

/* 初始化和清理函数 */
xc_runtime_t* xc_init(void);
void xc_shutdown(void);

/* 垃圾回收函数 */
void xc_gc_collect(void);
xc_object_t* xc_gc_retain(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj);

/* 类型检查函数 */
int xc_is_null(xc_runtime_t *rt, xc_object_t *obj);
int xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj);
int xc_is_number(xc_runtime_t *rt, xc_object_t *obj);
int xc_is_string(xc_runtime_t *rt, xc_object_t *obj);
int xc_is_array(xc_runtime_t *rt, xc_object_t *obj);
int xc_is_object(xc_runtime_t *rt, xc_object_t *obj);
int xc_is_function(xc_runtime_t *rt, xc_object_t *obj);

/* 值访问函数 */
int xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj);
double xc_number_value(xc_runtime_t *rt, xc_object_t *obj);
const char* xc_string_value(xc_runtime_t *rt, xc_object_t *obj);

/* 创建函数 */
xc_object_t* xc_null_create(xc_runtime_t *rt);
xc_object_t* xc_boolean_create(xc_runtime_t *rt, int value);
xc_object_t* xc_number_create(xc_runtime_t *rt, double value);
xc_object_t* xc_string_create(xc_runtime_t *rt, const char *value);
xc_object_t* xc_array_create(xc_runtime_t *rt);
xc_object_t* xc_array_create_with_capacity(xc_runtime_t *rt, size_t initial_capacity);
xc_object_t* xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count);

/* 数组操作函数 */
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *arr);
xc_object_t* xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index);
void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value);
void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t* xc_array_pop(xc_runtime_t *rt, xc_object_t *arr);
void xc_array_unshift(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t* xc_array_shift(xc_runtime_t *rt, xc_object_t *arr);
xc_object_t* xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end);
xc_object_t* xc_array_concat(xc_runtime_t *rt, xc_object_t *arr1, xc_object_t *arr2);
xc_object_t* xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator);
int xc_array_index_of(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
int xc_array_index_of_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index);

#endif /* XC_COMPAT_H */ 