/*
 * xc_runtime_internal.h - XC internal runtime structures
 * This file contains internal implementation details of the runtime system
 */
#ifndef XC_INTERNAL_H
#define XC_INTERNAL_H

#include "xc.h"
#include <setjmp.h>

/* Forward declarations */
typedef struct xc_object xc_object_t;
typedef struct xc_type xc_type_t;
typedef struct xc_exception_frame xc_exception_frame_t;
typedef struct xc_runtime_extended xc_runtime_extended_t;
typedef struct xc_stack_trace xc_stack_trace_t;
typedef struct xc_exception xc_exception_t;

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
    struct xc_type *type;     /* Pointer to type descriptor */
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

/* Cast runtime to extended runtime */
#define XC_RUNTIME_EXT(rt) ((xc_runtime_extended_t *)(rt))

/* 
 * Extended runtime structure with GC context
 * This extends the basic runtime structure with GC-specific fields
 */
typedef struct xc_runtime_extended {
    xc_runtime_t base;        /* Base runtime structure */
    void *gc_context;         /* Garbage collector context */
    xc_type_t *type_handlers[256]; /* Type handlers for different object types */
    xc_exception_frame_t *exception_frame; /* Current exception frame */
    
    /* Builtin types */
    xc_type_t *null_type;     /* Type for null objects */
    xc_type_t *boolean_type;  /* Type for boolean objects */
    xc_type_t *number_type;   /* Type for number objects */
    xc_type_t *string_type;   /* Type for string objects */
    xc_type_t *array_type;    /* Type for array objects */
    xc_type_t *object_type;   /* Type for generic objects */
    xc_type_t *function_type; /* Type for function objects */
    xc_type_t *error_type;    /* Type for error objects */
} xc_runtime_extended_t;

/* Function object structure */
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
//WARNING, DO NOT USE MACRO (wrong paradiam!)
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

#endif /* XC_INTERNAL_H */
