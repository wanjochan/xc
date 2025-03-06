/*
 * libxc.h - XC运行时库的公共API头文件 (完全展开版)
 */
#ifndef LIBXC_H
#define LIBXC_H

/* 包含必要的标准C库头文件 */
#include "cosmopolitan.h"

/* XC运行时库的公共API */
/* 保留区间 (16-31)：为未来的基础类型预留 */
/* 内部类型 (32-63)：运行时内部使用的类型 */
/* 用户自定义类型 (64-127)：通过API注册的类型 */
/* 扩展类型 (128-255)：为未来扩展预留 */
/* 对象颜色状态 - 用于三色标记法 */
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
    jmp_buf env; /* 保存的环境 */
    xc_val catch_func; /* catch处理器 */
    xc_val finally_func; /* finally处理器 */
    struct xc_exception_handler* prev; /* 链接到前一个处理器 */
} xc_exception_handler_t;
/* 类型生命周期管理结构 */
typedef struct {
    xc_initializer_func initializer; /* 初始化函数 */
    xc_cleaner_func cleaner; /* 清理函数 */
    xc_creator_func creator; /* 创建函数 */
    xc_destroy_func destroyer; /* 销毁函数 */
    xc_allocator_func allocator; /* 内存分配函数 */
    xc_marker_func marker; /* GC标记函数 */
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
 * xc_types.h - XC type system header file
 */
/*
 * XC - 极致轻量的C运行时引擎
 */
/*
 * xc_object.h - XC object structure definition
 */
/*
 * XC - 极致轻量的C运行时引擎
 */
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
/* Cast runtime to extended runtime */
/* Type flags */
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
/*
 * Type IDs are defined in xc.h
 * We use those definitions instead of redefining them here
 */
/*
 * Type registration functions
 */
void xc_register_null_type(xc_runtime_t *rt);
void xc_register_boolean_type(xc_runtime_t *rt);
void xc_register_number_type(xc_runtime_t *rt);
void xc_register_string_type(xc_runtime_t *rt);
void xc_register_array_type(xc_runtime_t *rt);
void xc_register_object_type(xc_runtime_t *rt);
void xc_register_function_type(xc_runtime_t *rt);
void xc_register_error_type(xc_runtime_t *rt);
/*
 * Type creation functions
 */
xc_object_t *xc_null_create(xc_runtime_t *rt);
xc_object_t *xc_boolean_create(xc_runtime_t *rt, _Bool value);
xc_object_t *xc_number_create(xc_runtime_t *rt, double value);
xc_object_t *xc_string_create(xc_runtime_t *rt, const char *value);
xc_object_t *xc_string_create_len(xc_runtime_t *rt, const char *value, size_t len);
xc_object_t *xc_array_create(xc_runtime_t *rt);
xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t capacity);
xc_object_t *xc_object_create(xc_runtime_t *rt);
xc_object_t *xc_function_create(xc_runtime_t *rt, xc_function_ptr_t fn, void *user_data);
/*
 * Type conversion functions
 */
_Bool xc_to_boolean(xc_runtime_t *rt, xc_object_t *obj);
double xc_to_number(xc_runtime_t *rt, xc_object_t *obj);
const char *xc_to_string(xc_runtime_t *rt, xc_object_t *obj);
/*
 * Type checking functions
 */
_Bool xc_is_null(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_number(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_string(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_object(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_function(xc_runtime_t *rt, xc_object_t *obj);
_Bool xc_is_error(xc_runtime_t *rt, xc_object_t *obj);
/*
 * Type value access
 */
_Bool xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj);
double xc_number_value(xc_runtime_t *rt, xc_object_t *obj);
const char *xc_string_value(xc_runtime_t *rt, xc_object_t *obj);
size_t xc_string_length(xc_runtime_t *rt, xc_object_t *obj);
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *obj);
xc_object_t *xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index);
void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value);
void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_pop(xc_runtime_t *rt, xc_object_t *arr);
xc_object_t *xc_object_get(xc_runtime_t *rt, xc_object_t *obj, const char *key);
void xc_object_set(xc_runtime_t *rt, xc_object_t *obj, const char *key, xc_object_t *value);
_Bool xc_object_has(xc_runtime_t *rt, xc_object_t *obj, const char *key);
void xc_object_delete(xc_runtime_t *rt, xc_object_t *obj, const char *key);
xc_object_t *xc_function_call(xc_runtime_t *rt, xc_object_t *func, xc_object_t *this_obj, size_t argc, xc_object_t **argv);
/*
 * Type iteration
 */
void xc_array_foreach(xc_runtime_t *rt, xc_object_t *arr, void (*callback)(xc_runtime_t *rt, size_t index, xc_object_t *value, void *user_data), void *user_data);
void xc_object_foreach(xc_runtime_t *rt, xc_object_t *obj, void (*callback)(xc_runtime_t *rt, const char *key, xc_object_t *value, void *user_data), void *user_data);
/*
 * Type comparison
 */
_Bool xc_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
_Bool xc_strict_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
int xc_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
/*
 * xc_gc.h - XC garbage collector header file
 */
/*
 * XC - 极致轻量的C运行时引擎
 */
/*
 * xc_object.h - XC object structure definition
 */
/* Garbage collector color marks for tri-color marking */
/* GC configuration structure */
typedef struct xc_gc_config {
    size_t initial_heap_size; /* Initial size of the heap in bytes */
    size_t max_heap_size; /* Maximum size of the heap in bytes */
    double growth_factor; /* Heap growth factor when resizing */
    double gc_threshold; /* Memory usage threshold to trigger GC */
    size_t max_alloc_before_gc; /* Maximum number of allocations before forced GC */
} xc_gc_config_t;
/* Default GC configuration */
/* GC statistics structure */
typedef struct xc_gc_stats {
    size_t heap_size; /* Current heap size in bytes */
    size_t used_memory; /* Used memory in bytes */
    size_t total_allocated; /* Total allocated objects since start */
    size_t total_freed; /* Total freed objects since start */
    size_t gc_cycles; /* Number of GC cycles */
    double avg_pause_time_ms; /* Average GC pause time in milliseconds */
    double last_pause_time_ms; /* Last GC pause time in milliseconds */
} xc_gc_stats_t;
/* GC initialization and shutdown */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config);
void xc_gc_shutdown(xc_runtime_t *rt);
/* GC control functions */
void xc_gc_run(xc_runtime_t *rt);
void xc_gc_enable(xc_runtime_t *rt);
void xc_gc_disable(xc_runtime_t *rt);
_Bool xc_gc_is_enabled(xc_runtime_t *rt);
/* Memory management functions */
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id);
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj); /* Mark an object as reachable */
/* Reference management */
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj);
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj);
/* Root set management */
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr);
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr);
/* GC statistics */
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt);
void xc_gc_print_stats(xc_runtime_t *rt);
/*
 * xc_exception.h - XC exception handling system
 */
/*
 * XC - 极致轻量的C运行时引擎
 */
/*
 * xc_object.h - XC object structure definition
 */
/* Exception type constants */
/* Exception frame structure */
typedef struct xc_exception_frame {
    jmp_buf jmp; /* Jump buffer for setjmp/longjmp */
    struct xc_exception_frame *prev; /* Previous frame in the chain */
    xc_object_t *exception; /* Current exception */
    _Bool handled; /* Whether the exception was handled */
    const char *file; /* Source file where frame was created */
    int line; /* Line number where frame was created */
    void *finally_handler; /* Finally handler if any */
    void *finally_context; /* Context for finally handler */
} xc_exception_frame_t;
/* Stack trace entry structure */
typedef struct xc_stack_trace_entry {
    const char *function; /* Function name */
    const char *file; /* Source file */
    int line; /* Line number */
} xc_stack_trace_entry_t;
/* Stack trace structure */
typedef struct xc_stack_trace {
    xc_stack_trace_entry_t *entries; /* Array of stack trace entries */
    size_t count; /* Number of entries */
    size_t capacity; /* Capacity of entries array */
} xc_stack_trace_t;
/* Exception object structure (extends xc_object_t) */
typedef struct xc_exception {
    xc_object_t base; /* Base object header */
    int type; /* Exception type */
    char *message; /* Exception message */
    xc_stack_trace_t *stack_trace; /* Stack trace */
    struct xc_exception *cause; /* Cause exception (if chained) */
} xc_exception_t;
/* Exception handling macros */
/* Exception API functions */
void xc_exception_init(xc_runtime_t *rt);
void xc_exception_shutdown(xc_runtime_t *rt);
xc_object_t *xc_exception_create(xc_runtime_t *rt, int type, const char *message);
xc_object_t *xc_exception_create_with_cause(xc_runtime_t *rt, int type, const char *message, xc_object_t *cause);
void xc_exception_throw(xc_runtime_t *rt, xc_object_t *exception);
int xc_exception_get_type(xc_runtime_t *rt, xc_object_t *exception);
const char *xc_exception_get_message(xc_runtime_t *rt, xc_object_t *exception);
xc_object_t *xc_exception_get_cause(xc_runtime_t *rt, xc_object_t *exception);
xc_stack_trace_t *xc_exception_get_stack_trace(xc_runtime_t *rt, xc_object_t *exception);
void xc_stack_trace_print(xc_runtime_t *rt, xc_stack_trace_t *stack_trace);
char *xc_stack_trace_to_string(xc_runtime_t *rt, xc_stack_trace_t *stack_trace);
void xc_exception_rethrow(xc_runtime_t *rt);
void xc_exception_clear(xc_runtime_t *rt);
/* Pre-defined exception creation helpers */
xc_object_t *xc_exception_create_error(xc_runtime_t *rt, const char *message);
xc_object_t *xc_exception_create_syntax_error(xc_runtime_t *rt, const char *message);
xc_object_t *xc_exception_create_type_error(xc_runtime_t *rt, const char *message);
xc_object_t *xc_exception_create_reference_error(xc_runtime_t *rt, const char *message);
xc_object_t *xc_exception_create_range_error(xc_runtime_t *rt, const char *message);
xc_object_t *xc_exception_create_memory_error(xc_runtime_t *rt, const char *message);
xc_object_t *xc_exception_create_internal_error(xc_runtime_t *rt, const char *message);
/*
 * xc_std_console.h - 控制台标准库头文件
 */
/*
 * XC - 极致轻量的C运行时引擎
 */
/* 获取Console对象 */
xc_val xc_std_get_console(void);
/* 初始化Console库 */
void xc_std_console_initialize(void);
/* 清理Console库 */
void xc_std_console_cleanup(void);
/*
 * xc_std_math.h - 数学标准库头文件
 */
/*
 * XC - 极致轻量的C运行时引擎
 */
/* 获取Math对象 */
xc_val xc_std_get_math(void);
/* 初始化Math库 */
void xc_std_math_initialize(void);
/* 清理Math库 */
void xc_std_math_cleanup(void);

#endif /* LIBXC_H */
