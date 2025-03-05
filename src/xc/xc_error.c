/*
 * xc_error.c - 错误类型实现
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


typedef int xc_error_code_t;
/* 错误类型数据结构 */
typedef struct {
    xc_error_code_t code;      // 错误代码
    char message[128];         // 错误消息，固定长度
    xc_val stack_trace;        // 栈跟踪
    xc_val cause;              // 原因异常（异常链）
} xc_error_t;

/* 错误类型方法 */
static xc_val error_to_string(xc_val self, xc_val arg) {
    xc_error_t* error = (xc_error_t*)self;
    return xc.create(XC_TYPE_STRING, error->message);
}

/* 获取错误代码 */
static xc_val error_get_code(xc_val self, xc_val arg) {
    xc_error_t* error = (xc_error_t*)self;
    return xc.create(XC_TYPE_NUMBER, (double)error->code);
}

/* 获取错误消息 */
static xc_val error_get_message(xc_val self, xc_val arg) {
    xc_error_t* error = (xc_error_t*)self;
    return xc.create(XC_TYPE_STRING, error->message);
}

/* 获取栈跟踪 */
static xc_val error_get_stack_trace(xc_val self, xc_val arg) {
    xc_error_t* error = (xc_error_t*)self;
    return error->stack_trace ? error->stack_trace : xc.create(XC_TYPE_ARRAY, 0);
}

/* 设置栈跟踪 */
static xc_val error_set_stack_trace(xc_val self, xc_val arg) {
    if (!arg || !xc.is(arg, XC_TYPE_ARRAY)) {
        return xc.create(XC_TYPE_ERROR, XC_ERR_TYPE, "栈跟踪必须是数组");
    }
    
    xc_error_t* error = (xc_error_t*)self;
    error->stack_trace = arg;
    return xc.create(XC_TYPE_NULL);
}

/* 获取原因异常 */
static xc_val error_get_cause(xc_val self, xc_val arg) {
    xc_error_t* error = (xc_error_t*)self;
    return error->cause ? error->cause : xc.create(XC_TYPE_NULL);
}

/* 设置原因异常 */
static xc_val error_set_cause(xc_val self, xc_val arg) {
    if (arg && !xc.is(arg, XC_TYPE_ERROR)) {
        return xc.create(XC_TYPE_ERROR, XC_ERR_TYPE, "cause必须是错误类型");
    }
    
    xc_error_t* error = (xc_error_t*)self;
    error->cause = arg;
    return xc.create(XC_TYPE_NULL);
}

/* 错误类型的 GC 标记方法 */
static void error_marker(xc_val self, void (*mark_func)(xc_val)) {
    xc_error_t* error = (xc_error_t*)self;
    if (error->stack_trace) {
        mark_func(error->stack_trace);
    }
    if (error->cause) {
        mark_func(error->cause);
    }
}

/* 错误类型的销毁方法 */
static int error_destroy(xc_val self) {
    // 没有需要释放的资源
    return 0;
}

/* 错误类型初始化 */
static void error_initialize(void) {
    // 注册方法
    xc.register_method(XC_TYPE_ERROR, "toString", error_to_string);
    xc.register_method(XC_TYPE_ERROR, "getCode", error_get_code);
    xc.register_method(XC_TYPE_ERROR, "getMessage", error_get_message);
    xc.register_method(XC_TYPE_ERROR, "getStackTrace", error_get_stack_trace);
    xc.register_method(XC_TYPE_ERROR, "setStackTrace", error_set_stack_trace);
    xc.register_method(XC_TYPE_ERROR, "getCause", error_get_cause);
    xc.register_method(XC_TYPE_ERROR, "setCause", error_set_cause);
}

/* 错误类型创建函数 */
static xc_val error_creator(int type, va_list args) {
    // 获取错误代码和消息
    xc_error_code_t code = va_arg(args, xc_error_code_t);
    const char* message = va_arg(args, const char*);
    
    // 分配错误对象
    xc_val obj = xc.alloc_object(type, sizeof(xc_error_t));
    if (!obj) return NULL;
    
    // 初始化错误对象
    xc_error_t* error = (xc_error_t*)obj;
    error->code = code;
    error->stack_trace = NULL;
    error->cause = NULL;  // 初始化cause为NULL
    
    // 复制消息，确保不会溢出
    if (message) {
        strncpy(error->message, message, sizeof(error->message) - 1);
        error->message[sizeof(error->message) - 1] = '\0';
    } else {
        error->message[0] = '\0';
    }
    
    return obj;
}

/* 注册错误类型 */
int _xc_type_error = XC_TYPE_ERROR;

__attribute__((constructor)) static void register_error_type(void) {
    /* 定义类型生命周期管理接口 */
    static xc_type_lifecycle_t lifecycle = {
        .initializer = error_initialize,
        .cleaner = NULL,
        .creator = error_creator,
        .destroyer = error_destroy,
        .marker = error_marker,
        .allocator = NULL
    };
    
    /* 注册类型 */
    _xc_type_error = xc.register_type("error", &lifecycle);
    // error_initialize();
}

/* 创建错误对象 */
xc_val xc_error_create(xc_error_code_t code, const char* message) {
    return xc.create(XC_TYPE_ERROR, code, message);
}

/* 获取错误代码 */
xc_error_code_t xc_error_get_code(xc_val error) {
    if (!xc.is(error, XC_TYPE_ERROR)) return XC_ERR_NONE;
    return ((xc_error_t*)error)->code;
}

/* 获取错误消息 */
const char* xc_error_get_message(xc_val error) {
    if (!xc.is(error, XC_TYPE_ERROR)) return NULL;
    return ((xc_error_t*)error)->message;
}

/* 设置错误栈跟踪 */
void xc_error_set_stack_trace(xc_val error, xc_val stack_trace) {
    if (!xc.is(error, XC_TYPE_ERROR) || !stack_trace) return;
    xc.call(error, "setStackTrace", stack_trace);
}

/* 获取错误栈跟踪 */
xc_val xc_error_get_stack_trace(xc_val error) {
    if (!xc.is(error, XC_TYPE_ERROR)) return NULL;
    return xc.call(error, "getStackTrace", NULL);
}
