/*
 * xc_error.h - 错误类型定义
 */

#ifndef XC_ERROR_H
#define XC_ERROR_H

#include "xc.h"
#include "xc_object.h"

/* 错误代码定义 */
#define XC_ERR_NONE 0
#define XC_ERR_GENERIC 1        /* 通用错误 */
#define XC_ERR_TYPE 2           /* 类型错误 */
#define XC_ERR_VALUE 3          /* 值错误 */
#define XC_ERR_INDEX 4          /* 索引错误 */
#define XC_ERR_KEY 5            /* 键错误 */
#define XC_ERR_ATTRIBUTE 6      /* 属性错误 */
#define XC_ERR_NAME 7           /* 名称错误 */
#define XC_ERR_SYNTAX 8         /* 语法错误 */
#define XC_ERR_RUNTIME 9        /* 运行时错误 */
#define XC_ERR_MEMORY 10        /* 内存错误 */
#define XC_ERR_IO 11            /* IO错误 */
#define XC_ERR_NOT_IMPLEMENTED 12  /* 未实现错误 */
#define XC_ERR_INVALID_ARGUMENT 13  /* 无效参数错误 */
#define XC_ERR_ASSERTION 14     /* 断言错误 */
#define XC_ERR_USER 15          /* 用户自定义错误 */

/* 错误类型数据结构 */
typedef int xc_error_code_t;

/* 错误创建函数 */
xc_object_t *xc_error_create(xc_runtime_t *rt, xc_error_code_t code, const char* message);

/* 错误代码获取函数 */
xc_error_code_t xc_error_get_code(xc_runtime_t *rt, xc_object_t *error);

/* 错误消息获取函数 */
const char* xc_error_get_message(xc_runtime_t *rt, xc_object_t *error);

/* 设置栈跟踪 */
void xc_error_set_stack_trace(xc_runtime_t *rt, xc_object_t *error, xc_object_t *stack_trace);

/* 获取栈跟踪 */
xc_object_t *xc_error_get_stack_trace(xc_runtime_t *rt, xc_object_t *error);

/* 设置错误原因 */
void xc_error_set_cause(xc_runtime_t *rt, xc_object_t *error, xc_object_t *cause);

/* 获取错误原因 */
xc_object_t *xc_error_get_cause(xc_runtime_t *rt, xc_object_t *error);

/* 类型检查 */
bool xc_is_error(xc_runtime_t *rt, xc_object_t *obj);

#endif /* XC_ERROR_H */ 