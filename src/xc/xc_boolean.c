/*
 * xc_bool.c - 布尔类型实现
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 布尔类型数据结构 */
typedef struct {
    int value;  // 使用int而不是char以避免对齐问题
} xc_boolean_t;

/* 内部函数 - 获取布尔值 */
static int get_boolean_value(xc_val obj) {
    if (!obj || !xc.is(obj, XC_TYPE_BOOL)) return 0;
    xc_boolean_t* data = (xc_boolean_t*)obj;
    return data->value;
}

/* 布尔类型方法 */
static xc_val boolean_to_string(xc_val self, xc_val arg) {
    int value = get_boolean_value(self);
    return xc.create(XC_TYPE_STRING, value ? "true" : "false");
}

static xc_val boolean_not(xc_val self, xc_val arg) {
    int value = get_boolean_value(self);
    return xc.create(XC_TYPE_BOOL, !value);
}

static xc_val boolean_and(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_BOOL)) {
        return NULL;
    }
    
    int value1 = get_boolean_value(self);
    int value2 = get_boolean_value(arg);
    
    return xc.create(XC_TYPE_BOOL, value1 && value2);
}

static xc_val boolean_or(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_BOOL)) {
        return NULL;
    }
    
    int value1 = get_boolean_value(self);
    int value2 = get_boolean_value(arg);
    
    return xc.create(XC_TYPE_BOOL, value1 || value2);
}

static xc_val boolean_xor(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_BOOL)) {
        return NULL;
    }
    
    int value1 = get_boolean_value(self);
    int value2 = get_boolean_value(arg);
    
    return xc.create(XC_TYPE_BOOL, value1 ^ value2);
}

/* 布尔类型的GC标记方法 - 布尔类型没有引用其他对象 */
static void boolean_marker(xc_val self, void (*mark_func)(xc_val)) {
    /* 布尔类型没有引用其他对象 */
}

/* 布尔类型的销毁方法 */
static int boolean_destroy(xc_val self) {
    /* 只需要释放对象本身，value是直接存储的 */
    return 1;
}

/* 布尔类型的初始化方法 */
static void boolean_initialize(void) {
    /* 注册类型方法 */
    xc.register_method(XC_TYPE_BOOL, "toString", boolean_to_string);
    xc.register_method(XC_TYPE_BOOL, "not", boolean_not);
    xc.register_method(XC_TYPE_BOOL, "and", boolean_and);
    xc.register_method(XC_TYPE_BOOL, "or", boolean_or);
    xc.register_method(XC_TYPE_BOOL, "xor", boolean_xor);
}

/* 布尔类型的创建方法 */
static xc_val boolean_creator(int type, va_list args) {
    /* 获取布尔值参数 */
    int value = va_arg(args, int);
    
    /* 分配布尔对象 */
    xc_val obj = xc.alloc_object(type, sizeof(xc_boolean_t));
    if (!obj) return NULL;
    
    /* 设置布尔值 */
    xc_boolean_t* data = (xc_boolean_t*)obj;
    data->value = value ? 1 : 0;
    
    return obj;
}

/* 布尔类型生命周期管理结构 */
static xc_type_lifecycle_t boolean_lifecycle = {
    .initializer = boolean_initialize,
    .cleaner = NULL,
    .creator = boolean_creator,
    .destroyer = boolean_destroy,
    .allocator = NULL,
    .marker = boolean_marker
};

/* Register boolean type - 通过xc_init调用 */
void xc_boolean_register(void) {
    /* 注册类型 */
    xc.register_type("boolean", &boolean_lifecycle);
}
