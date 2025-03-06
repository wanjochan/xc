/*
 * xc_exception.h - XC exception handling system
 */
#ifndef XC_EXCEPTION_H
#define XC_EXCEPTION_H

#include "xc.h"
#include "xc_object.h"
#include <setjmp.h>

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

/* Exception handling macros */
#define XC_TRY(rt) \
    do { \
        xc_exception_frame_t __frame; \
        __frame.prev = (rt)->exception_frame; \
        __frame.exception = NULL; \
        __frame.handled = false; \
        __frame.file = __FILE__; \
        __frame.line = __LINE__; \
        __frame.finally_handler = NULL; \
        __frame.finally_context = NULL; \
        (rt)->exception_frame = &__frame; \
        if (setjmp(__frame.jmp) == 0) {

#define XC_CATCH(rt, var) \
        } else { \
            __frame.handled = true; \
            xc_object_t *var = __frame.exception;

#define XC_FINALLY(rt, handler, context) \
        } \
        __frame.finally_handler = (handler); \
        __frame.finally_context = (context);

#define XC_END_TRY(rt) \
        if (__frame.finally_handler) { \
            ((void (*)(xc_runtime_t *, void *))__frame.finally_handler)((rt), __frame.finally_context); \
        } \
        (rt)->exception_frame = __frame.prev; \
        if (__frame.exception && !__frame.handled) { \
            xc_exception_throw(rt, __frame.exception); \
        } \
    } while (0)

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

#endif /* XC_EXCEPTION_H */
