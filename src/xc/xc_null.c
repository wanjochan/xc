/*
 * xc_null.c - NULL类型实现
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

/* NULL类型方法 */
static xc_val null_to_string(xc_val self, xc_val arg) {
    return xc.create(XC_TYPE_STRING, "null");
}

static xc_val null_is_null(xc_val self, xc_val arg) {
    return xc.create(XC_TYPE_BOOL, 1);
}

/* NULL类型的GC标记方法 - 无需标记任何内容 */
static void null_mark(xc_val self, void (*mark_func)(xc_val)) {
    /* NULL类型没有引用其他对象，无需标记 */
}

/* NULL类型的销毁方法 - 无需特殊清理 */
static int null_destroy(xc_val self) {
    /* NULL类型没有特殊资源需要释放 */
    return 0;
}

/* NULL类型初始化 - 对xc.c可见但不在头文件中声明 */
void xc_null_initialize(void) {
    /* 空值类型不需要特殊初始化 */
}

/* NULL类型清理 - 对xc.c可见但不在头文件中声明 */
void xc_null_cleanup(void) {
    /* 注册方法 */
    // xc.register_method(XC_TYPE_NULL, "toString", null_to_string);
    // xc.register_method(XC_TYPE_NULL, "isNull", null_is_null);
}

/* NULL类型的创建函数 */
static xc_val null_create(int type, va_list args) {
    /* 空值通常是单例，直接返回NULL或特定常量 */
    return NULL;
}

/* 创建空值 */
xc_val xc_null_create(int type, ...) {
    va_list args;
    va_start(args, type);
    xc_val result = null_create(type, args);
    va_end(args);
    return result;
}

/* 注册NULL类型 */
__attribute__((constructor)) static void register_null_type(void) {
    /* 定义类型生命周期管理接口 */
    xc_type_lifecycle_t lifecycle = {
        .initializer = xc_null_initialize,
        .cleaner = xc_null_cleanup,
        .creator = null_create,
        .destroyer = null_destroy,
        .marker = null_mark,
        .allocator = NULL
    };
    
    /* 注册类型 */
    xc.register_type("null", &lifecycle);
    
    // xc_null_initialize();
}