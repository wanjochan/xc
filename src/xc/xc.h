#ifndef XC_H
#define XC_H

//DO NOT change this file until PM approved
//@ref build_libxc.sh: xc.h => libxc.h for libxc.a

#include "cosmopolitan.h" //will change to infrax in later versionss

#define XC_FALSE 0
#define XC_TRUE 1

/* 类型ID定义 */
/* 核心类型区间 */
#define XC_TYPE_UNDEFINED    -1
#define XC_TYPE_NULL         2
#define XC_TYPE_BOOL         3
#define XC_TYPE_NUMBER       4
#define XC_TYPE_STRING       5
#define XC_TYPE_EXCEPTION    6
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
typedef xc_val (*xc_allocator_func)(size_t size);
typedef xc_val (*xc_method_func)(xc_val self, xc_val arg);

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

    int (*register_type)(const char* name, xc_type_lifecycle_t* lifecycle);
    int (*get_type_id)(const char* name);
    
    //runtime, calling stack
    char (*register_method)(int type, const char* func_name, xc_method_func native_func);
    xc_val (*call)(xc_val obj, const char* method, ...);
    xc_val (*dot)(xc_val obj, const char* key, ...);
    xc_val (*invoke)(xc_val func, int argc, ...);
    
    //exception handling
    xc_val (*try_catch_finally)(xc_val try_func, xc_val catch_func, xc_val finally_func);
    void (*throw)(xc_val error);
    void (*throw_with_rethrow)(xc_val error);
    void (*set_uncaught_exception_handler)(xc_val handler);
    xc_val (*get_current_error)(void);
    void (*clear_error)(void);
} xc_runtime_t;

/* xc global instance */
extern xc_runtime_t xc;

#define XC_REQUIRES(x) typeof(x) *const xc_requires_##x = &(x)

#endif /* XC_H */
