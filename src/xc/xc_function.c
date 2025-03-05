/*
 * xc_function.c - 函数类型实现
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 函数处理器类型定义 */
typedef xc_val (*xc_function_handler)(xc_val this_obj, int argc, xc_val* argv, xc_val closure);

/* 前向声明 */
xc_val xc_function_create(xc_function_handler handler, int arg_count, xc_val closure);

/* 函数类型数据结构定义 - 仅在本文件中可见 */
typedef struct {
    xc_function_handler handler;  /* 函数处理器 */
    xc_val this_obj;              /* 函数绑定的this对象 */
    xc_val closure;              /* 闭包环境 */
    int arg_count;               /* 参数数量 */
} xc_function_t;

/* 函数类型方法 */
static xc_val function_to_string(xc_val self, xc_val arg) {
    return xc.create(XC_TYPE_STRING, "function");
}
//TODO 这个跟 xc.invoke 高度一样，考虑合并
static xc_val function_invoke(xc_val self, xc_val arg) {
    xc_function_t* func = (xc_function_t*)self;
//TODO 入栈、异常处理
    
    /* 直接调用函数处理器 */
    return func->handler(func->this_obj, 1, &arg, func->closure);
}

static xc_val function_bind(xc_val self, xc_val this_obj) {
    xc_function_t* func = (xc_function_t*)self;
    func->this_obj = this_obj;
    return self;
}

/* 函数类型的GC标记方法 */
static void function_marker(xc_val self, void (*mark_func)(xc_val)) {
    xc_function_t* func = (xc_function_t*)self;
    if (func->closure) {
        mark_func(func->closure);
    }
    if (func->this_obj) {
        mark_func(func->this_obj);
    }
}

/* 函数类型的销毁方法 */
static int function_destroy(xc_val self) {
    xc_function_t* func = (xc_function_t*)self;
    if (func->closure) {
        /* 闭包对象由GC负责回收 */
        func->closure = NULL;
    }
    if (func->this_obj) {
        /* this对象由GC负责回收 */
        func->this_obj = NULL;
    }
    return 0;
}

/* 函数类型的初始化方法 */
static void function_initialize(void) {
    /* 注册类型方法 */
    xc.register_method(XC_TYPE_FUNC, "toString", function_to_string);
    xc.register_method(XC_TYPE_FUNC, "invoke", function_invoke);
    xc.register_method(XC_TYPE_FUNC, "bind", function_bind);
}

/* 函数类型的创建方法 */
static xc_val function_creator(int type, va_list args) {
    /* 获取函数处理器和闭包对象 */
    xc_function_handler handler = va_arg(args, xc_function_handler);
    int arg_count = va_arg(args, int);
    xc_val closure = va_arg(args, xc_val);
    
    /* 分配函数对象内存 */
    xc_val obj = xc.alloc_object(type, sizeof(xc_function_t));
    if (!obj) return NULL;
    
    /* 初始化函数对象 */
    xc_function_t* func = (xc_function_t*)obj;
    func->handler = handler;
    func->arg_count = arg_count;
    func->closure = closure;  /* 闭包对象由GC负责管理 */
    func->this_obj = NULL;  /* 默认不绑定this对象 */
    
    return obj;
}

/* 函数类型的调用方法 */
xc_val xc_function_invoke(xc_val obj, xc_val this_obj, int argc, xc_val* argv) {
    if (!obj || !xc.is(obj, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    /* 获取函数对象 */
    xc_function_t* func = (xc_function_t*)obj;
    
    /* 调用函数处理器 */
    return func->handler(this_obj ? this_obj : func->this_obj, argc, argv, func->closure);
}

int _xc_type_func = XC_TYPE_FUNC;

/* 函数类型生命周期 */
static xc_type_lifecycle_t function_lifecycle = {
    .initializer = function_initialize,
    .cleaner = NULL,
    .creator = function_creator,
    .destroyer = function_destroy,
    .marker = function_marker,
    .allocator = NULL
};

/* 注册函数类型 */
void xc_function_register(void) {
    /* 注册类型 */
    _xc_type_func = xc.register_type("function", &function_lifecycle);
}

/* 创建函数对象 */
xc_val xc_function_create(xc_function_handler handler, int arg_count, xc_val closure) {
    if (!handler) return NULL;
    
    return xc.create(XC_TYPE_FUNC, handler, arg_count, closure);
}

/* 获取函数闭包 */
xc_val xc_function_get_closure(xc_val obj) {
    if (!xc.is(obj, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    xc_function_t* func = (xc_function_t*)obj;
    return func->closure;
}