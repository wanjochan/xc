/*
 * xc_number.c - 数值类型实现(F64)
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 前向声明 */
xc_val xc_number_create(int type, ...);

int _xc_type_number = XC_TYPE_NUMBER;

/* 数值类型数据结构定义 - 仅在本文件中可见 */
typedef struct {
    double value;//F64
} xc_number_t;

/* 数值类型方法 */
static xc_val number_to_string(xc_val self, xc_val arg) {
    xc_number_t* num = (xc_number_t*)self;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%g", num->value);
    return xc.create(XC_TYPE_STRING, buffer);
}

static xc_val number_add(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL;
    }
    
    xc_number_t* num1 = (xc_number_t*)self;
    xc_number_t* num2 = (xc_number_t*)arg;
    
    return xc.create(XC_TYPE_NUMBER, num1->value + num2->value);
}

static xc_val number_subtract(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL;
    }
    
    xc_number_t* num1 = (xc_number_t*)self;
    xc_number_t* num2 = (xc_number_t*)arg;
    
    return xc.create(XC_TYPE_NUMBER, num1->value - num2->value);
}

static xc_val number_multiply(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL;
    }
    
    double value1 = *(double*)self;
    double value2 = *(double*)arg;
    
    return xc.create(XC_TYPE_NUMBER, value1 * value2);
}

static xc_val number_divide(xc_val self, xc_val arg) {
    if (!xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL;
    }
    
    double value1 = *(double*)self;
    double value2 = *(double*)arg;
    
    if (value2 == 0.0) {
        return NULL;  /* 除数为零 */
    }
    
    return xc.create(XC_TYPE_NUMBER, value1 / value2);
}

/* 数值类型的GC标记方法 - 数值类型没有引用其他对象，所以实现为空 */
static void number_marker(xc_val self, void (*mark_func)(xc_val)) {
    /* 数值类型没有引用其他对象 */
}

/* 数值类型的销毁方法 */
static int number_destroy(xc_val self) {
    /* 数值类型的内存由GC负责回收 */
    return 0;
}

/* 数值类型的初始化方法 */
static void number_initialize(void) {
    /* 注册类型方法 */
    xc.register_method(_xc_type_number, "toString", number_to_string);
    xc.register_method(_xc_type_number, "add", number_add);
    xc.register_method(_xc_type_number, "subtract", number_subtract);
    xc.register_method(_xc_type_number, "multiply", number_multiply);
    xc.register_method(_xc_type_number, "divide", number_divide);
}

/* 数值类型的创建方法 */
static xc_val number_creator(int type, va_list args) {
    /* 分配数值对象 */
    xc_number_t* obj = (xc_number_t*)xc.alloc_object(XC_TYPE_NUMBER, sizeof(xc_number_t));
    if (!obj) {
        return NULL;
    }
    
    double value = va_arg(args, double);
    
    /* 设置值 */
    obj->value = value;
    
    return (xc_val)obj;
}

/* 数值类型生命周期 */
static xc_type_lifecycle_t number_lifecycle = {
    .initializer = number_initialize,
    .cleaner = NULL,
    .creator = number_creator,
    .destroyer = number_destroy,
    .marker = number_marker,
    .allocator = NULL
};

/* 注册数值类型 */
void xc_number_register(void) {
    _xc_type_number = xc.register_type("number", &number_lifecycle);
}

/* 创建数值实例 - 直接接受参数版本 */
static xc_val number_create(int type, va_list args) {
    /* 分配数值对象 */
    xc_number_t* obj = (xc_number_t*)xc.alloc_object(XC_TYPE_NUMBER, sizeof(xc_number_t));
    if (!obj) {
        return NULL;
    }
    
    double value = va_arg(args, double);
    
    /* 设置值 */
    obj->value = value;
    
    return (xc_val)obj;
}

/* 创建数值 */
xc_val xc_number_create(int type, ...) {
    va_list args;
    va_start(args, type);
    xc_val result = number_create(type, args);
    va_end(args);
    return result;
}

/* 获取数值 */
double xc_number_get_value(xc_val obj) {
    xc_number_t* data = (xc_number_t*)obj;
    return data->value;
}
