/*
 * xc_runtime_internal.h - XC internal runtime structures
 * This file contains internal implementation details of the runtime system
 */
#ifndef XC_INTERNAL_H
#define XC_INTERNAL_H

#include "xc.h"

/* Forward declarations */
typedef struct xc_object xc_object_t;
typedef struct xc_closure xc_closure_t;
typedef struct xc_exception_frame xc_exception_frame_t;
typedef struct xc_gc_context xc_gc_context_t;

/* Type registry entry */
typedef struct xc_type_entry {
    const char* name;
    int id;
    xc_type_lifecycle_t lifecycle;
    struct xc_type_entry* next;
} xc_type_entry_t;

/* Type registry structure */
typedef struct {
    int count;
    xc_type_entry_t* buckets[256];
} xc_type_registry_t;

/* Global type handlers and instances */
extern xc_type_lifecycle_t *xc_type_handlers[256];
extern xc_type_registry_t type_registry;

////NOTES： 其实可以后面设计一个 xc.get_type_by_id()来获得的，当然会慢一些，所以先考虑好再说
/* Primitive type handlers */
extern xc_type_lifecycle_t *xc_null_type;
extern xc_type_lifecycle_t *xc_boolean_type;
extern xc_type_lifecycle_t *xc_number_type;
extern xc_type_lifecycle_t *xc_string_type;
extern xc_type_lifecycle_t *xc_array_type;
extern xc_type_lifecycle_t *xc_object_type;
extern xc_type_lifecycle_t *xc_function_type;
extern xc_type_lifecycle_t *xc_error_type;

/* Forward declarations for GC types and functions */
typedef struct xc_gc_config xc_gc_config_t;
typedef struct xc_gc_stats xc_gc_stats_t;

/* 添加全局变量声明 */
extern void *xc_gc_context;
extern xc_exception_frame_t *xc_exception_frame;

/* GC function declarations */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config);
void xc_gc_shutdown(xc_runtime_t *rt);
void xc_gc_run(xc_runtime_t *rt);
void xc_gc_enable(xc_runtime_t *rt);
void xc_gc_disable(xc_runtime_t *rt);
bool xc_gc_is_enabled(xc_runtime_t *rt);
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id);
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_val(xc_val obj);
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj);
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr);
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr);
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt);
void xc_gc_print_stats(xc_runtime_t *rt);
void xc_gc_release_object(xc_val obj);
void xc_release(xc_val obj);

/* 
 * Function pointer type for XC functions
 */
typedef xc_val (*xc_function_ptr_t)(xc_runtime_t *rt, xc_val this_obj, int argc, xc_val *argv);

/* 
 * Basic object structure for all XC objects
 * This is the common header for all objects managed by the GC
 */
typedef struct xc_object {
    size_t size;              /* Total size of the object in bytes */
    int type_id;              /* 类型ID，直接使用XC_TYPE_*常量 */
    int ref_count;            /* Reference count for manual memory management */
    int gc_color;             /* GC mark color (white, gray, black, permanent) */
    struct xc_object *gc_next; /* Next object in the GC list */
    /* Object data follows this header */
} xc_object_t;

/* Type flags */
#define XC_TYPE_PRIMITIVE  0x0001  /* Primitive type (number, string, etc) */
#define XC_TYPE_COMPOSITE 0x0002   /* Composite type (array, object) */
#define XC_TYPE_CALLABLE  0x0004   /* Callable type (function) */
#define XC_TYPE_INTERNAL  0x0008   /* Internal type */

/*
 * Type handler structure
 * Contains function pointers for type-specific operations
 */
typedef struct xc_type {
    const char *name;         /* Type name */
    int flags;               /* Type flags */
    void (*free)(xc_runtime_t *rt, xc_object_t *obj);  /* Free type-specific resources */
    void (*mark)(xc_runtime_t *rt, xc_object_t *obj);  /* Mark referenced objects */
    bool (*equal)(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);  /* Equality comparison */
    int (*compare)(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);  /* Ordering comparison */
} xc_type_t;

/* Object property structure */
typedef struct xc_property {
    xc_object_t *key;    /* String key */
    xc_object_t *value;  /* Any value */
} xc_property_t;

/* Object data structure */
typedef struct xc_object_data_t {
    xc_object_t base;          /* Must be first */
    xc_property_t *properties; /* Array of properties */
    size_t count;             /* Number of properties */
    size_t capacity;          /* Allocated capacity */
    xc_object_t *prototype;   /* Prototype object */
} xc_object_data_t;

/* 
 * Function structure for XC functions
 */
typedef struct xc_function_t {
    xc_object_t base;          /* Must be first */
    xc_function_ptr_t handler; /* Function handler */
    xc_object_t *closure;      /* Closure environment */
    xc_object_t *this_obj;     /* Bound this value */
} xc_function_t;

/* Array object structure */
typedef struct xc_array_t {
    xc_object_t base;     /* Must be first */
    xc_object_t **items;  /* Array of object pointers */
    size_t length;        /* Current number of items */
    size_t capacity;      /* Allocated capacity */
} xc_array_t;

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

/* Exception type constants */
#define XC_EXCEPTION_TYPE_ERROR       0   /* Generic Error */
#define XC_EXCEPTION_TYPE_SYNTAX      1   /* Syntax Error */
#define XC_EXCEPTION_TYPE_TYPE        2   /* Type Error */
#define XC_EXCEPTION_TYPE_REFERENCE   3   /* Reference Error */
#define XC_EXCEPTION_TYPE_RANGE       4   /* Range Error */
#define XC_EXCEPTION_TYPE_MEMORY      5   /* Memory Error */
#define XC_EXCEPTION_TYPE_INTERNAL    6   /* Internal Error */
#define XC_EXCEPTION_TYPE_USER        100 /* User-defined Exceptions start here */

/* Exception frame structure */
typedef struct xc_exception_frame {
    jmp_buf jmp;                        /* Jump buffer for setjmp/longjmp */
    struct xc_exception_frame *prev;    /* Previous frame in the chain */
    xc_object_t *exception;             /* Current exception */
    bool handled;                       /* Whether the exception was handled */
    const char *file;                   /* Source file where frame was created */
    int line;                           /* Line number where frame was created */
    void *finally_handler;              /* Finally handler if any */
    void *finally_context;              /* Context for finally handler */
} xc_exception_frame_t;

/* Stack trace entry structure */
typedef struct xc_stack_trace_entry {
    const char *function;               /* Function name */
    const char *file;                   /* Source file */
    int line;                           /* Line number */
} xc_stack_trace_entry_t;

/* Stack trace structure */
typedef struct xc_stack_trace {
    xc_stack_trace_entry_t *entries;    /* Array of stack trace entries */
    size_t count;                       /* Number of entries */
    size_t capacity;                    /* Capacity of entries array */
} xc_stack_trace_t;

/* Exception object structure (extends xc_object_t) */
typedef struct xc_exception {
    xc_object_t base;                   /* Base object header */
    int type;                           /* Exception type */
    char *message;                      /* Exception message */
    xc_stack_trace_t *stack_trace;      /* Stack trace */
    struct xc_exception *cause;         /* Cause exception (if chained) */
} xc_exception_t;

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

/* Type registration helper */
int xc_register_type(const char *name, xc_type_lifecycle_t *lifecycle);

/*
 * Type creation functions
 */
xc_object_t *xc_null_create(xc_runtime_t *rt);
xc_object_t *xc_boolean_create(xc_runtime_t *rt, bool value);
xc_object_t *xc_number_create(xc_runtime_t *rt, double value);
xc_object_t *xc_string_create(xc_runtime_t *rt, const char *value);
xc_object_t *xc_string_create_len(xc_runtime_t *rt, const char *value, size_t len);
xc_object_t *xc_array_create(xc_runtime_t *rt);
xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t capacity);
xc_object_t *xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count);
xc_object_t *xc_object_create(xc_runtime_t *rt);
xc_object_t *xc_function_create(xc_runtime_t *rt, xc_function_ptr_t fn, xc_object_t *closure);
xc_object_t *xc_function_get_closure(xc_runtime_t *rt, xc_object_t *func);

/*
 * Type conversion functions
 */
bool xc_to_boolean(xc_runtime_t *rt, xc_object_t *obj);
double xc_to_number(xc_runtime_t *rt, xc_object_t *obj);
const char *xc_to_string(xc_runtime_t *rt, xc_object_t *obj);

/*
 * Type checking functions
 */
bool xc_is_null(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_number(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_string(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_object(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_function(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_error(xc_runtime_t *rt, xc_object_t *obj);

/*
 * Type value access
 */
bool xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj);
double xc_number_value(xc_runtime_t *rt, xc_object_t *obj);
const char *xc_string_value(xc_runtime_t *rt, xc_object_t *obj);
size_t xc_string_length(xc_runtime_t *rt, xc_object_t *obj);
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *obj);
xc_object_t *xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index);
void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value);
void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_pop(xc_runtime_t *rt, xc_object_t *arr);
void xc_array_unshift(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_shift(xc_runtime_t *rt, xc_object_t *arr);
xc_object_t *xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end);
xc_object_t *xc_array_concat(xc_runtime_t *rt, xc_object_t *arr1, xc_object_t *arr2);
xc_object_t *xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator);
int xc_array_index_of(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
int xc_array_index_of_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index);
xc_object_t *xc_object_get(xc_runtime_t *rt, xc_object_t *obj, const char *key);
void xc_object_set(xc_runtime_t *rt, xc_object_t *obj, const char *key, xc_object_t *value);
bool xc_object_has(xc_runtime_t *rt, xc_object_t *obj, const char *key);
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
bool xc_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
bool xc_strict_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
int xc_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);

/* Exception handling macros */
//WARNING, DO NOT USE MACRO
// #define XC_TRY(rt) \
//     do { \
//         xc_exception_frame_t __frame; \
//         __frame.prev = (rt)->exception_frame; \
//         __frame.exception = NULL; \
//         __frame.handled = false; \
//         __frame.file = __FILE__; \
//         __frame.line = __LINE__; \
//         __frame.finally_handler = NULL; \
//         __frame.finally_context = NULL; \
//         (rt)->exception_frame = &__frame; \
//         if (setjmp(__frame.jmp) == 0) {

// #define XC_CATCH(rt, var) \
//         } else { \
//             __frame.handled = true; \
//             xc_object_t *var = __frame.exception;

// #define XC_FINALLY(rt, handler, context) \
//         } \
//         __frame.finally_handler = (handler); \
//         __frame.finally_context = (context);

// #define XC_END_TRY(rt) \
//         if (__frame.finally_handler) { \
//             ((void (*)(xc_runtime_t *, void *))__frame.finally_handler)((rt), __frame.finally_context); \
//         } \
//         (rt)->exception_frame = __frame.prev; \
//         if (__frame.exception && !__frame.handled) { \
//             xc_exception_throw(rt, __frame.exception); \
//         } \
//     } while (0)

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

/* Function for invoking functions */
xc_val xc_function_invoke(xc_val func, xc_val this_obj, int argc, xc_val* argv);

void xc_gc(void);


/* Garbage collector color marks for tri-color marking */
#define XC_GC_WHITE      0   /* Object is not reachable (candidate for collection) */
#define XC_GC_GRAY       1   /* Object is reachable but its children haven't been scanned */
#define XC_GC_BLACK      2   /* Object is reachable and its children have been scanned */
#define XC_GC_PERMANENT  3   /* Object is permanently reachable (never collected) */

/* GC configuration structure */
typedef struct xc_gc_config {
    size_t initial_heap_size;   /* Initial size of the heap in bytes */
    size_t max_heap_size;       /* Maximum size of the heap in bytes */
    double growth_factor;       /* Heap growth factor when resizing */
    double gc_threshold;        /* Memory usage threshold to trigger GC */
    size_t max_alloc_before_gc; /* Maximum number of allocations before forced GC */
} xc_gc_config_t;

/* Default GC configuration */
#define XC_GC_DEFAULT_CONFIG { \
    .initial_heap_size = 1024 * 1024, \
    .max_heap_size = 1024 * 1024 * 1024, \
    .growth_factor = 1.5, \
    .gc_threshold = 0.7, \
    .max_alloc_before_gc = 10000 \
}

/* GC statistics structure */
typedef struct xc_gc_stats {
    size_t heap_size;           /* Current heap size in bytes */
    size_t used_memory;         /* Used memory in bytes */
    size_t total_allocated;     /* Total allocated objects since start */
    size_t total_freed;         /* Total freed objects since start */
    size_t gc_cycles;           /* Number of GC cycles */
    double avg_pause_time_ms;   /* Average GC pause time in milliseconds */
    double last_pause_time_ms;  /* Last GC pause time in milliseconds */
} xc_gc_stats_t;

/* Forward declarations of internal type structures */
typedef struct xc_array_t xc_array_t;
typedef struct xc_object_data_t xc_object_data_t;
typedef struct xc_function_t xc_function_t;

/* Internal GC context structure */
typedef struct xc_gc_context {
    xc_gc_config_t config;           /* GC configuration */
    size_t heap_size;                /* Current heap size in bytes */
    size_t used_memory;              /* Used memory in bytes */
    size_t allocation_count;         /* Allocations since last GC */
    size_t total_allocated;          /* Total allocated objects */
    size_t total_freed;              /* Total freed objects */
    size_t gc_cycles;                /* Number of GC cycles */
    double total_pause_time_ms;      /* Total GC pause time in ms */
    bool enabled;                    /* Whether GC is enabled */
    
    /* Root set */
    xc_object_t ***roots;            /* Array of pointers to root objects */
    size_t root_count;               /* Number of roots */
    size_t root_capacity;            /* Capacity of roots array */
    
    /* Object lists for tri-color marking */
    xc_object_t *white_list;         /* White objects (candidates for collection) */
    xc_object_t *gray_list;          /* Gray objects (reachable but not scanned) */
    xc_object_t *black_list;         /* Black objects (reachable and scanned) */
} xc_gc_context_t;

/* Function declarations for GC */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config);
void xc_gc_shutdown(xc_runtime_t *rt);
void xc_gc_run(xc_runtime_t *rt);
void xc_gc_enable(xc_runtime_t *rt);
void xc_gc_disable(xc_runtime_t *rt);
bool xc_gc_is_enabled(xc_runtime_t *rt);
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id);
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_val(xc_val obj);
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj);
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr);
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr);
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt);
void xc_gc_print_stats(xc_runtime_t *rt);
void xc_gc_release_object(xc_val obj);
void xc_release(xc_val obj);

/* 错误代码 */
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

/* 函数处理器类型定义 */
typedef xc_val (*xc_function_handler)(xc_val this_obj, int argc, xc_val* argv, xc_val closure);

/* 执行栈帧结构 */
typedef struct xc_stack_frame {
    const char* func_name;         /* 函数名称 */
    const char* file_name;         /* 文件名称 */
    int line_number;               /* 行号 */
    struct xc_stack_frame* prev;   /* 上一帧 */
} xc_stack_frame_t;

/* 异常处理器结构 - 内部增强版本 */
typedef struct xc_exception_handler_internal {
    jmp_buf env;                      /* 保存的环境 */
    xc_val catch_func;                /* catch处理器 */
    xc_val finally_func;              /* finally处理器 */
    struct xc_exception_handler_internal* prev; /* 链接到前一个处理器 */
    xc_val current_exception;         /* 当前处理的异常 */
    bool has_caught;                  /* 是否已经捕获过异常 */
} xc_exception_handler_internal_t;

/* 统一线程本地状态 */
static __thread struct {
    /* 栈相关 */
    xc_stack_frame_t* top;         /* 栈顶 */
    int depth;                     /* 栈深度 */
    /* 异常相关 */
    xc_exception_handler_internal_t* current;  /* 当前异常处理器 */
    xc_val current_error;          /* 当前错误 */
    bool in_try_block;             /* 是否在try块中 */
    xc_val uncaught_handler;       /* 未捕获异常处理器 */
} _xc_thread_state = {0};

/* 类型注册表结构 */
#define TYPE_HASH_SIZE 64

/* 对象头结构 - 每个对象的隐藏前缀 */
typedef struct {
    int type;           /* 类型ID */
    unsigned int flags;  /* 标志位 */
    int ref_count;       /* 引用计数 */
    void* next_gc;       /* GC链表下一项 */
    size_t size;         /* 对象大小（包括头部） */
    const char* type_name; /* 类型名称（便于调试） */
    unsigned char color;   /* 对象颜色，用于三色标记法 */
} xc_header_t;

/* 标志位定义 */
#define XC_FLAG_ROOT     (1 << 0)  /* GC根对象 */
#define XC_FLAG_FINALIZE (1 << 1)  /* 需要终结 */

/* 从对象获取头部 */
#define XC_HEADER(obj) ((xc_header_t*)((char*)(obj) - sizeof(xc_header_t)))

/* 从头部获取对象 */
#define XC_OBJECT(header) ((xc_val)((char*)(header) + sizeof(xc_header_t)))

/* 线程本地GC状态 */
static __thread struct {
    void* gc_first;         /* 本线程GC链表头 */
    size_t total_memory;    /* 本线程分配的内存总量 */
    size_t gc_threshold;    /* 本线程GC阈值 */
    char initialized;       /* 线程本地状态是否已初始化 */
    void* stack_bottom;     /* 栈底指针 */
    size_t gray_count;      /* 灰色对象计数 */
    xc_header_t** gray_stack; /* 灰色对象栈 */
    size_t gray_capacity;   /* 灰色对象栈容量 */
    size_t allocation_count; /* 分配计数，用于触发自动GC */
} _thread_gc = {0};

/* 全局状态结构 */
typedef struct {
    //char initialized;
    /* 类型方法表 */
    struct {
        const char* name;
        xc_val (*func)(xc_val, ...);
        int next;  /* 链表下一个方法索引 */
    } methods[256];  /* 方法池 */
    int method_count;
    int method_heads[16];  /* 每个类型的方法链表头 */
    xc_type_registry_t type_registry;
} xc_state_t;

/* 全局状态 */
static xc_state_t _state = {0};

static void push_stack_frame(const char* func_name, const char* file_name, int line_number);
static void pop_stack_frame(void);
static void thread_cleanup(void);
static void throw_internal(xc_val error, bool allow_rethrow);
static void throw(xc_val error);
static void throw_with_rethrow(xc_val error);
static xc_val create(int type, ...);
static int type_of(xc_val val);
static int is(xc_val val, int type);
static xc_val invoke(xc_val func, int argc, ...);
static xc_val xc_dot(xc_val obj, const char* key, ...);
static xc_val call(xc_val obj, const char* method, ...);
static void gc_mark_object(xc_val obj);
static void gc_mark_stack(void);
static void gc_mark_roots(void);
static void gc_sweep(void);
static void gc_mark_gray(xc_header_t* header);
static void gc_scan_gray(void);

#endif /* XC_INTERNAL_H */
