/*
 * libxc.h - XC运行时库的公共API头文件 (完全展开版)
 */
#ifndef LIBXC_H
#define LIBXC_H

/* 包含必要的标准C库头文件 */
#include "cosmopolitan.h"

/* XC类型定义和常量 */
#define XC_FALSE 0
#define XC_TRUE 1

/* 类型ID定义 */
/* 核心类型区间 */
#define XC_TYPE_UNDEFINED    -1
#define XC_TYPE_NULL         2
#define XC_TYPE_BOOL         3
#define XC_TYPE_NUMBER       4
#define XC_TYPE_STRING       5
#define XC_TYPE_ERROR        6
#define XC_TYPE_FUNC         7
#define XC_TYPE_ARRAY        8
#define XC_TYPE_OBJECT       9
#define XC_TYPE_VM           10

/* 保留区间 (16-31)：为未来的基础类型预留 */

/* 内部类型 (32-63)：运行时内部使用的类型 */
#define XC_TYPE_INTERNAL_BEGIN  32
#define XC_TYPE_ITERATOR        32
#define XC_TYPE_REGEXP          33
#define XC_TYPE_DATE            34
#define XC_TYPE_BUFFER          35
#define XC_TYPE_INTERNAL_END    63

/* 用户自定义类型 (64-127)：通过API注册的类型 */
#define XC_TYPE_USER_BEGIN      64
#define XC_TYPE_USER_END        127

/* 扩展类型 (128-255)：为未来扩展预留 */
#define XC_TYPE_EXTENSION_BEGIN 128
#define XC_TYPE_EXTENSION_END   255

/* 对象颜色状态 - 用于三色标记法 */
#define XC_COLOR_WHITE       0  /* 未标记，可能是垃圾 */
#define XC_COLOR_GRAY        1  /* 已标记但引用未处理 */
#define XC_COLOR_BLACK       2  /* 已标记且引用已处理 */

/* 胖指针 值类型 */
typedef void* xc_val;

/* 函数类型定义 */
typedef void (*xc_initializer_func)(void);
typedef void (*xc_cleaner_func)(void);
typedef xc_val (*xc_creator_func)(int type, va_list args);
typedef int (*xc_destroy_func)(xc_val obj);
typedef void (*xc_marker_func)(xc_val obj, void (*mark_func)(xc_val));
typedef void* (*xc_allocator_func)(size_t size);
typedef xc_val (*xc_method_func)(xc_val self, xc_val arg);

/* 异常处理器结构 */
typedef struct xc_exception_handler {
    jmp_buf env;                     /* 保存的环境 */
    xc_val catch_func;               /* catch处理器 */
    xc_val finally_func;             /* finally处理器 */
    struct xc_exception_handler* prev; /* 链接到前一个处理器 */
} xc_exception_handler_t;

/* 类型生命周期管理结构 */
typedef struct {
    xc_initializer_func initializer;  /* 初始化函数 */
    xc_cleaner_func cleaner;          /* 清理函数 */
    xc_creator_func creator;          /* 创建函数 */
    xc_destroy_func destroyer;        /* 销毁函数 */
    xc_allocator_func allocator;      /* 内存分配函数 */
    xc_marker_func marker;            /* GC标记函数 */
} xc_type_lifecycle_t;

/* 运行时接口结构 */
typedef struct xc_runtime_t {
    /* type */
    xc_val (*alloc_object)(int type, ...);
    xc_val (*create)(int type, ...);
    int (*type_of)(xc_val obj);
    int (*is)(xc_val obj, int type);
    
    /* 类型管理 */
    int (*register_type)(const char* name, xc_type_lifecycle_t* lifecycle);
    int (*get_type_id)(const char* name);
    
    /* 运行时：原生包装器、调用栈、异常处理 */
    char (*register_method)(int type, const char* func_name, xc_method_func native_func);
    xc_val (*call)(xc_val obj, const char* method, ...);
    xc_val (*dot)(xc_val obj, const char* key, ...);
    xc_val (*invoke)(xc_val func, int argc, ...);
    
    /* 异常处理 */
    xc_val (*try_catch_finally)(xc_val try_func, xc_val catch_func, xc_val finally_func);
    void (*throw)(xc_val error);
    void (*throw_with_rethrow)(xc_val error);
    void (*set_uncaught_exception_handler)(xc_val handler);
    xc_val (*get_current_error)(void);
    void (*clear_error)(void);
    
    /* 垃圾回收 */
    void (*gc)(void);
    
    /* 引用计数 */
    xc_val (*retain)(xc_val obj);
    void (*release)(xc_val obj);
    void (*mark)(xc_val obj);
    
} xc_runtime_t;

/* 全局运行时对象 */
extern xc_runtime_t xc;

/* 
 * Function pointer type for XC functions
 */
typedef xc_val (*xc_function_ptr_t)(xc_runtime_t *rt, xc_val this_obj, int argc, xc_val *argv);

/* 
 * Basic object structure for all XC objects
 * This is the common header for all objects managed by the GC
 */
typedef struct xc_object {
    size_t size; /* Total size of the object in bytes */
    struct xc_type *type; /* Pointer to type descriptor */
    int ref_count; /* Reference count for manual memory management */
    int gc_color; /* GC mark color (white, gray, black, permanent) */
    struct xc_object *gc_next; /* Next object in the GC list */
    /* Object data follows this header */
} xc_object_t;

/*
 * Type handler structure
 * Contains function pointers for type-specific operations
 */
typedef struct xc_type {
    const char *name; /* Type name */
    int flags; /* Type flags */
    void (*free)(xc_runtime_t *rt, xc_object_t *obj); /* Free type-specific resources */
    void (*mark)(xc_runtime_t *rt, xc_object_t *obj); /* Mark referenced objects */
    _Bool (*equal)(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b); /* Equality comparison */
    int (*compare)(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b); /* Ordering comparison */
} xc_type_t;

/* Forward declaration for exception frame */
typedef struct xc_exception_frame xc_exception_frame_t;

/* 
 * Extended runtime structure with GC context
 * This extends the basic runtime structure with GC-specific fields
 */
typedef struct xc_runtime_extended {
    xc_runtime_t base; /* Base runtime structure */
    void *gc_context; /* Garbage collector context */
    xc_type_t *type_handlers[256]; /* Type handlers for different object types */
    xc_exception_frame_t *exception_frame; /* Current exception frame */
    /* Builtin types */
    xc_type_t *null_type; /* Type for null objects */
    xc_type_t *boolean_type; /* Type for boolean objects */
    xc_type_t *number_type; /* Type for number objects */
    xc_type_t *string_type; /* Type for string objects */
    xc_type_t *array_type; /* Type for array objects */
    xc_type_t *object_type; /* Type for generic objects */
    xc_type_t *function_type; /* Type for function objects */
    xc_type_t *error_type; /* Type for error objects */
} xc_runtime_extended_t;

/* 获取Console对象 */
xc_val xc_std_get_console(void);

/* 初始化Console库 */
void xc_std_console_initialize(void);

/* 清理Console库 */
void xc_std_console_cleanup(void);

/* 获取Math对象 */
xc_val xc_std_get_math(void);

/* 初始化Math库 */
void xc_std_math_initialize(void);

/* 清理Math库 */
void xc_std_math_cleanup(void);

#endif /* LIBXC_H */
