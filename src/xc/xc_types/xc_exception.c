#include "../xc.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c
#include "../xc_internal.h"

/* Error type descriptor */
static xc_type_t error_type = {0};

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

/* Initialize the exception handling system */
void xc_exception_init(xc_runtime_t *rt) {
    xc_exception_frame = NULL;
    
    /* Register the error type */
    xc_register_error_type(rt);
}

/* Shutdown exception subsystem */
void xc_exception_shutdown(xc_runtime_t *rt) {
    /* Nothing to do here */
}

/* Create a new exception object */
xc_object_t *xc_exception_create(xc_runtime_t *rt, int type, const char *message) {
    return (xc_object_t *)xc_exception_create_internal(rt, type, message, NULL);
}

/* Create a new exception object with a cause */
xc_object_t *xc_exception_create_with_cause(xc_runtime_t *rt, int type, const char *message, xc_object_t *cause) {
    return (xc_object_t *)xc_exception_create_internal(rt, type, message, cause);
}

/* Throw an exception */
void xc_exception_throw(xc_runtime_t *rt, xc_object_t *exception) {
    if (!xc_exception_frame) {
        fprintf(stderr, "Uncaught exception: ");
        /* Print exception details */
        // ...
        exit(1);
    }
    
    xc_exception_frame->exception = exception;
    
    longjmp(xc_exception_frame->jmp, 1);
}

/* Get exception type */
int xc_exception_get_type(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || exception->type != xc_error_type) return -1;
    xc_exception_t *exc = (xc_exception_t *)exception;
    return exc->type;
}

/* Get exception message */
const char *xc_exception_get_message(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || exception->type != xc_error_type) return "Not an exception";
    xc_exception_t *exc = (xc_exception_t *)exception;
    return exc->message;
}

/* Get exception cause */
xc_object_t *xc_exception_get_cause(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || exception->type != xc_error_type) return NULL;
    xc_exception_t *exc = (xc_exception_t *)exception;
    return (xc_object_t *)exc->cause;
}

/* Get exception stack trace */
xc_stack_trace_t *xc_exception_get_stack_trace(xc_runtime_t *rt, xc_object_t *exception) {
    if (!exception || exception->type != xc_error_type) return NULL;
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

/* Error type handler - free function */
static void xc_error_free(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || obj->type != xc_error_type) return;
    
    xc_exception_t *error = (xc_exception_t *)obj;
    
    /* Free the message */
    if (error->message) {
        free(error->message);
    }
    
    /* Free the stack trace */
    if (error->stack_trace) {
        if (error->stack_trace->entries) {
            free(error->stack_trace->entries);
        }
        free(error->stack_trace);
    }
}

/* Error type handler - mark function */
static void xc_error_mark(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || obj->type != xc_error_type) return;
    
    xc_exception_t *error = (xc_exception_t *)obj;
    
    /* Mark the cause exception */
    if (error->cause) {
        xc_gc_mark(rt, (xc_object_t *)error->cause);
    }
}

/* Error type handler - to string function */
static xc_object_t *xc_error_to_string(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || obj->type != xc_error_type) return NULL;
    
    xc_exception_t *error = (xc_exception_t *)obj;
    
    /* Build a string representation */
    const char *type_str = "Error";
    switch (error->type) {
        case XC_EXCEPTION_TYPE_SYNTAX:    type_str = "SyntaxError"; break;
        case XC_EXCEPTION_TYPE_TYPE:      type_str = "TypeError"; break;
        case XC_EXCEPTION_TYPE_REFERENCE: type_str = "ReferenceError"; break;
        case XC_EXCEPTION_TYPE_RANGE:     type_str = "RangeError"; break;
        case XC_EXCEPTION_TYPE_MEMORY:    type_str = "MemoryError"; break;
        case XC_EXCEPTION_TYPE_INTERNAL:  type_str = "InternalError"; break;
    }
    
    const char *message = error->message ? error->message : "No message";
    
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
    // 如果已经注册，直接返回
    if (xc_error_type) return;
    
    // 初始化错误类型
    error_type.name = "error";
    error_type.flags = XC_TYPE_EXCEPTION;
    error_type.free = xc_error_free;
    error_type.mark = xc_error_mark;
    error_type.equal = NULL;  // 错误对象不支持相等比较
    error_type.compare = NULL;  // 错误对象不支持排序比较
    
    // 注册类型
    xc_error_type = &error_type;
}
