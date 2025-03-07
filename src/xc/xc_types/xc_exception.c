#include <stdio.h>
#include <stdlib.h>
#include "../xc_internal.h"
#include <setjmp.h>

/* 异常帧链表头 - 使用外部声明而不是定义 */
extern xc_exception_frame_t *xc_exception_frame;

/* Forward declarations */
static void xc_error_free(xc_runtime_t *rt, xc_object_t *obj);
static void xc_error_mark(xc_runtime_t *rt, xc_object_t *obj);
static bool error_equal(xc_val a, xc_val b);
static int error_compare(xc_val a, xc_val b);
static xc_val error_creator(int type, va_list args);

/* Type descriptor for error type */
static xc_type_lifecycle_t error_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = error_creator,
    .destroyer = (xc_destroy_func)xc_error_free,
    .marker = (xc_marker_func)xc_error_mark,
    .allocator = NULL,
    .name = "error",
    .equal = (bool (*)(xc_val, xc_val))error_equal,
    .compare = (int (*)(xc_val, xc_val))error_compare,
    .flags = 0
};

static xc_type_lifecycle_t *error_type_ptr = NULL;

/* Create a stack trace entry */
static xc_stack_trace_entry_t xc_stack_trace_entry_create(const char *function, const char *file, int line) {
    xc_stack_trace_entry_t entry;
    entry.function = function ? strdup(function) : strdup("unknown");
    entry.file = file ? strdup(file) : strdup("unknown");
    entry.line = line;
    return entry;
}

/* Create a stack trace */
static xc_stack_trace_t *xc_stack_trace_create(xc_runtime_t *rt) {
    xc_stack_trace_t *stack_trace = (xc_stack_trace_t *)malloc(sizeof(xc_stack_trace_t));
    if (!stack_trace) return NULL;
    
    stack_trace->entries = NULL;
    stack_trace->count = 0;
    stack_trace->capacity = 0;
    
    return stack_trace;
}

/* Add an entry to a stack trace */
static void xc_stack_trace_add_entry(xc_stack_trace_t *stack_trace, const char *function, const char *file, int line) {
    if (!stack_trace) return;
    
    /* Check if we need to expand the entries array */
    if (stack_trace->count >= stack_trace->capacity) {
        size_t new_capacity = stack_trace->capacity == 0 ? 16 : stack_trace->capacity * 2;
        xc_stack_trace_entry_t *new_entries = (xc_stack_trace_entry_t *)realloc(stack_trace->entries, new_capacity * sizeof(xc_stack_trace_entry_t));
        if (!new_entries) return;
        stack_trace->entries = new_entries;
        stack_trace->capacity = new_capacity;
    }
    
    /* Add the entry */
    stack_trace->entries[stack_trace->count++] = xc_stack_trace_entry_create(function, file, line);
}

/* Free a stack trace */
static void xc_stack_trace_free(xc_stack_trace_t *stack_trace) {
    if (!stack_trace) return;
    
    /* Free all entries */
    for (size_t i = 0; i < stack_trace->count; i++) {
        free((void *)stack_trace->entries[i].function);
        free((void *)stack_trace->entries[i].file);
    }
    
    /* Free the entries array */
    free(stack_trace->entries);
    
    /* Free the stack trace structure */
    free(stack_trace);
}

/* Capture the current stack trace */
xc_stack_trace_t *xc_stack_trace_capture(xc_runtime_t *rt) {
    xc_exception_frame_t *frame = xc_exception_frame;
    
    xc_stack_trace_t *trace = (xc_stack_trace_t *)malloc(sizeof(xc_stack_trace_t));
    if (!trace) {
        return NULL;
    }
    
    trace->entries = NULL;
    trace->count = 0;
    trace->capacity = 0;
    
    while (frame) {
        if (trace->count >= trace->capacity) {
            size_t new_capacity = trace->capacity == 0 ? 8 : trace->capacity * 2;
            xc_stack_trace_entry_t *new_entries = (xc_stack_trace_entry_t *)realloc(
                trace->entries, new_capacity * sizeof(xc_stack_trace_entry_t));
            if (!new_entries) {
                return trace;
            }
            trace->entries = new_entries;
            trace->capacity = new_capacity;
        }
        
        trace->entries[trace->count].function = frame->file;
        trace->entries[trace->count].file = frame->file;
        trace->entries[trace->count].line = frame->line;
        trace->count++;
        
        frame = frame->prev;
    }
    
    return trace;
}

/* Create a new exception object */
static xc_exception_t *xc_exception_create_internal(xc_runtime_t *rt, int type, const char *message, xc_object_t *cause) {
    /* Allocate memory for the exception object */
    xc_exception_t *exception = (xc_exception_t *)xc_gc_alloc(rt, sizeof(xc_exception_t), XC_TYPE_EXCEPTION);
    if (!exception) return NULL;
    
    /* Initialize the exception fields */
    exception->type = type;
    exception->message = message ? strdup(message) : NULL;
    exception->stack_trace = xc_stack_trace_capture(rt);
    exception->cause = (struct xc_exception *)cause;
    
    return (xc_exception_t *)exception;
}

/* 设置异常的cause（异常链） */
static xc_val xc_exception_set_cause(xc_val self, xc_val arg) {
    if (!self || ((xc_object_t *)self)->type_id != XC_TYPE_EXCEPTION) {
        return NULL;
    }
    
    xc_exception_t *exception = (xc_exception_t *)self;
    
    /* 只有当arg是异常对象时才设置cause */
    if (arg && ((xc_object_t *)arg)->type_id == XC_TYPE_EXCEPTION) {
        exception->cause = (struct xc_exception *)arg;
    }
    
    return self;
}

/* 获取异常的cause */
static xc_val xc_exception_get_cause_method(xc_val self, xc_val arg) {
    if (!self || ((xc_object_t *)self)->type_id != XC_TYPE_EXCEPTION) {
        return NULL;
    }
    
    xc_exception_t *exception = (xc_exception_t *)self;
    return (xc_val)exception->cause;
}

/* 获取异常的消息 */
static xc_val xc_exception_get_message_method(xc_val self, xc_val arg) {
    if (!self || ((xc_object_t *)self)->type_id != XC_TYPE_EXCEPTION) {
        return NULL;
    }
    
    xc_exception_t *exception = (xc_exception_t *)self;
    return xc_string_create(NULL, exception->message ? exception->message : "");
}

/* 获取异常的类型 */
static xc_val xc_exception_get_type_method(xc_val self, xc_val arg) {
    if (!self || ((xc_object_t *)self)->type_id != XC_TYPE_EXCEPTION) {
        return NULL;
    }
    
    xc_exception_t *exception = (xc_exception_t *)self;
    return xc_number_create(NULL, (double)exception->type);
}

/* 获取异常的堆栈跟踪 */
static xc_val xc_exception_get_stack_trace_method(xc_val self, xc_val arg) {
    if (!self || ((xc_object_t *)self)->type_id != XC_TYPE_EXCEPTION) {
        return NULL;
    }
    
    xc_exception_t *exception = (xc_exception_t *)self;
    
    if (!exception->stack_trace) {
        return NULL;
    }
    
    /* 创建一个字符串表示堆栈跟踪 */
    char *trace_str = xc_stack_trace_to_string(NULL, exception->stack_trace);
    if (!trace_str) {
        return NULL;
    }
    
    xc_val result = xc_string_create(NULL, trace_str);
    free(trace_str);
    
    return result;
}

/* 全局未捕获异常处理器 */
static xc_val g_uncaught_exception_handler = NULL;

/* Initialize the exception handling system */
void xc_exception_init(xc_runtime_t *rt) {
    xc_exception_frame = NULL;
    g_uncaught_exception_handler = NULL;
    
    /* Register the error type */
    xc_register_error_type(rt);
}

/* Shutdown exception subsystem */
void xc_exception_shutdown(xc_runtime_t *rt) {
    /* 清理未捕获异常处理器 */
    g_uncaught_exception_handler = NULL;
}

/* Create a new exception object */
xc_object_t *xc_exception_create(xc_runtime_t *rt, int type, const char *message) {
    return (xc_object_t *)xc_exception_create_internal(rt, type, message, NULL);
}

/* Create a new exception object with a cause */
xc_object_t *xc_exception_create_with_cause(xc_runtime_t *rt, int type, const char *message, xc_object_t *cause) {
    return (xc_object_t *)xc_exception_create_internal(rt, type, message, cause);
}

/* 设置未捕获异常处理器 */
void xc_set_uncaught_exception_handler(xc_runtime_t *rt, xc_val handler) {
    if (handler && ((xc_object_t *)handler)->type_id == XC_TYPE_FUNC) {
        g_uncaught_exception_handler = handler;
    } else {
        g_uncaught_exception_handler = NULL;
    }
}

/* 获取当前未捕获异常处理器 */
xc_val xc_get_uncaught_exception_handler(xc_runtime_t *rt) {
    return g_uncaught_exception_handler;
}

/* Throw an exception */
void xc_exception_throw(xc_runtime_t *rt, xc_object_t *exception) {
    if (!xc_exception_frame) {
        /* 没有异常处理框架，这是一个未捕获的异常 */
        
        /* 如果有未捕获异常处理器，调用它 */
        if (g_uncaught_exception_handler && ((xc_object_t *)g_uncaught_exception_handler)->type_id == XC_TYPE_FUNC) {
            xc_val args[1] = {exception};
            xc_function_invoke(g_uncaught_exception_handler, NULL, 1, args);
        } else {
            /* 没有处理器，打印异常信息并退出 */
            fprintf(stderr, "Uncaught exception: ");
            
            /* 打印异常详情 */
            if (exception && ((xc_object_t *)exception)->type_id == XC_TYPE_EXCEPTION) {
                xc_exception_t *exc = (xc_exception_t *)exception;
                fprintf(stderr, "%s\n", exc->message ? exc->message : "No message");
                
                /* 打印堆栈跟踪 */
                if (exc->stack_trace) {
                    xc_stack_trace_print(rt, exc->stack_trace);
                }
                
                /* 打印异常链 */
                xc_exception_t *cause = exc->cause;
                if (cause) {
                    fprintf(stderr, "Caused by: %s\n", cause->message ? cause->message : "No message");
                    if (cause->stack_trace) {
                        xc_stack_trace_print(rt, cause->stack_trace);
                    }
                }
            } else {
                fprintf(stderr, "Unknown exception\n");
            }
            
            exit(1);
        }
        return;
    }
    
    /* 设置当前异常帧的异常并进行longjmp */
    xc_exception_frame->exception = exception;
    
    longjmp(xc_exception_frame->jmp, 1);
}

/* Get exception type */
int xc_exception_get_type(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || ((xc_object_t *)exception)->type_id != XC_TYPE_EXCEPTION) return -1;
    xc_exception_t *exc = (xc_exception_t *)exception;
    return exc->type;
}

/* Get exception message */
const char *xc_exception_get_message(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || ((xc_object_t *)exception)->type_id != XC_TYPE_EXCEPTION) return "Not an exception";
    xc_exception_t *exc = (xc_exception_t *)exception;
    return exc->message;
}

/* Get exception cause */
xc_object_t *xc_exception_get_cause(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || ((xc_object_t *)exception)->type_id != XC_TYPE_EXCEPTION) return NULL;
    xc_exception_t *exc = (xc_exception_t *)exception;
    return (xc_object_t *)exc->cause;
}

/* Get exception stack trace */
xc_stack_trace_t *xc_exception_get_stack_trace(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || ((xc_object_t *)exception)->type_id != XC_TYPE_EXCEPTION) return NULL;
    xc_exception_t *exc = (xc_exception_t *)exception;
    return exc->stack_trace;
}

/* Print stack trace */
void xc_stack_trace_print(xc_runtime_t *rt, xc_stack_trace_t *stack_trace) {
    if (!stack_trace) return;
    
    printf("Stack trace:\n");
    for (size_t i = 0; i < stack_trace->count; i++) {
        printf("  at %s (%s:%d)\n", 
               stack_trace->entries[i].function, 
               stack_trace->entries[i].file, 
               stack_trace->entries[i].line);
    }
}

/* Convert stack trace to string */
char *xc_stack_trace_to_string(xc_runtime_t *rt, xc_stack_trace_t *stack_trace) {
    if (!stack_trace) return strdup("No stack trace");
    
    /* Calculate the length of the string */
    size_t length = 14; /* "Stack trace:\n" */
    for (size_t i = 0; i < stack_trace->count; i++) {
        length += 6; /* "  at " */
        length += strlen(stack_trace->entries[i].function);
        length += 2; /* " (" */
        length += strlen(stack_trace->entries[i].file);
        length += 1; /* ":" */
        length += 10; /* line number (assuming max 10 digits) */
        length += 2; /* ")\n" */
    }
    
    /* Allocate the string */
    char *str = (char *)malloc(length + 1);
    if (!str) return NULL;
    
    /* Build the string */
    char *ptr = str;
    ptr += sprintf(ptr, "Stack trace:\n");
    for (size_t i = 0; i < stack_trace->count; i++) {
        ptr += sprintf(ptr, "  at %s (%s:%d)\n", 
                      stack_trace->entries[i].function, 
                      stack_trace->entries[i].file, 
                      stack_trace->entries[i].line);
    }
    
    return str;
}

/* Rethrow the current exception */
void xc_exception_rethrow(xc_runtime_t *rt) {
    if (!xc_exception_frame || !xc_exception_frame->exception) {
        fprintf(stderr, "No current exception to rethrow\n");
        return;
    }
    
    xc_object_t *exception = xc_exception_frame->exception;
    xc_exception_frame->exception = NULL;
    
    xc_exception_throw(rt, exception);
}

/* Clear the current exception */
void xc_exception_clear(xc_runtime_t *rt) {
    if (xc_exception_frame) {
        xc_exception_frame->exception = NULL;
    }
}

/* Create a generic error exception */
xc_object_t *xc_exception_create_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_ERROR, message);
}

/* Create a syntax error exception */
xc_object_t *xc_exception_create_syntax_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_SYNTAX, message);
}

/* Create a type error exception */
xc_object_t *xc_exception_create_type_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_TYPE, message);
}

/* Create a reference error exception */
xc_object_t *xc_exception_create_reference_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_REFERENCE, message);
}

/* Create a range error exception */
xc_object_t *xc_exception_create_range_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_RANGE, message);
}

/* Create a memory error exception */
xc_object_t *xc_exception_create_memory_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_MEMORY, message);
}

/* Create an internal error exception */
xc_object_t *xc_exception_create_internal_error(xc_runtime_t *rt, const char *message) {
    return xc_exception_create(rt, XC_EXCEPTION_TYPE_INTERNAL, message);
}

/* Free an error object */
static void xc_error_free(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || ((xc_object_t *)obj)->type_id != XC_TYPE_EXCEPTION) return;
    
    xc_exception_t *exception = (xc_exception_t *)obj;
    
    /* Free the message */
    if (exception->message) {
        free(exception->message);
        exception->message = NULL;
    }
    
    /* Free the stack trace */
    if (exception->stack_trace) {
        xc_stack_trace_free(exception->stack_trace);
        exception->stack_trace = NULL;
    }
    
    /* Note: We don't free the cause, as it's managed by GC */
}

/* Mark an error object for GC */
static void xc_error_mark(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || ((xc_object_t *)obj)->type_id != XC_TYPE_EXCEPTION) return;
    
    xc_exception_t *exception = (xc_exception_t *)obj;
    
    /* Mark the cause if any */
    if (exception->cause) {
        xc_gc_mark(rt, (xc_object_t *)exception->cause);
    }
}

/* Convert an error to a string */
static xc_object_t *xc_error_to_string(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || ((xc_object_t *)obj)->type_id != XC_TYPE_EXCEPTION) return NULL;
    
    xc_exception_t *exception = (xc_exception_t *)obj;
    
    /* Build a string representation */
    const char *type_str = "Error";
    switch (exception->type) {
        case XC_EXCEPTION_TYPE_SYNTAX:    type_str = "SyntaxError"; break;
        case XC_EXCEPTION_TYPE_TYPE:      type_str = "TypeError"; break;
        case XC_EXCEPTION_TYPE_REFERENCE: type_str = "ReferenceError"; break;
        case XC_EXCEPTION_TYPE_RANGE:     type_str = "RangeError"; break;
        case XC_EXCEPTION_TYPE_MEMORY:    type_str = "MemoryError"; break;
        case XC_EXCEPTION_TYPE_INTERNAL:  type_str = "InternalError"; break;
    }
    
    const char *message = exception->message ? exception->message : "No message";
    
    /* Allocate string buffer */
    size_t len = strlen(type_str) + 2 + strlen(message) + 1;
    char *buffer = (char *)malloc(len);
    if (!buffer) return NULL;
    
    /* Format the string */
    sprintf(buffer, "%s: %s", type_str, message);
    
    /* Create and return a string object */
    xc_object_t *str_obj = xc_string_create(rt, buffer);
    free(buffer);
    return str_obj;
}

/* Register error type */
void xc_register_error_type(xc_runtime_t *rt) {
    if (error_type_ptr) return;
    
    error_type_ptr = &error_type;
    
    /* Register the error type */
    int type_id = xc_register_type("error", error_type_ptr);
    
    /* Register methods for the error type */
    rt->register_method(XC_TYPE_EXCEPTION, "setCause", xc_exception_set_cause);
    rt->register_method(XC_TYPE_EXCEPTION, "getCause", xc_exception_get_cause_method);
    rt->register_method(XC_TYPE_EXCEPTION, "getMessage", xc_exception_get_message_method);
    rt->register_method(XC_TYPE_EXCEPTION, "getType", xc_exception_get_type_method);
    rt->register_method(XC_TYPE_EXCEPTION, "getStackTrace", xc_exception_get_stack_trace_method);
}

/* Error comparison functions */
static bool error_equal(xc_val a, xc_val b) {
    // 错误对象只有在是同一个对象时才相等
    return a == b;
}

static int error_compare(xc_val a, xc_val b) {
    // 错误对象不支持排序比较，只返回内存地址比较
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

/* Error creator function */
static xc_val error_creator(int type, va_list args) {
    const char *message = va_arg(args, const char *);
    return (xc_val)xc_exception_create(NULL, XC_EXCEPTION_TYPE_INTERNAL, message);
}
