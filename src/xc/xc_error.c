/*
 * xc_error.c - Error type implementation
 */

#include "xc.h"
#include "xc_error.h"
#include "xc_object.h"
#include "xc_gc.h"
#include "xc_types/xc_types.h"

/* Error object structure */
typedef struct {
    xc_object_t base;          /* Must be first */
    xc_error_code_t code;      /* Error code */
    xc_object_t *message;      /* String message */
    xc_object_t *stack_trace;  /* Array of stack frames */
    xc_object_t *cause;        /* Cause error (error chain) */
} xc_error_t;

/* Error methods */
static void error_mark(xc_runtime_t *rt, xc_object_t *obj) {
    xc_error_t *err = (xc_error_t *)obj;
    if (err->message) {
        xc_gc_mark(rt, err->message);
    }
    if (err->stack_trace) {
        xc_gc_mark(rt, err->stack_trace);
    }
    if (err->cause) {
        xc_gc_mark(rt, err->cause);
    }
}

static void error_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_error_t *err = (xc_error_t *)obj;
    if (err->message) {
        xc_gc_free(rt, err->message);
    }
    if (err->stack_trace) {
        xc_gc_free(rt, err->stack_trace);
    }
    if (err->cause) {
        xc_gc_free(rt, err->cause);
    }
}

static bool error_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_error(rt, b)) {
        return false;
    }
    
    xc_error_t *err_a = (xc_error_t *)a;
    xc_error_t *err_b = (xc_error_t *)b;
    
    /* Compare error codes first */
    if (err_a->code != err_b->code) {
        return false;
    }
    
    /* Compare messages */
    return xc_equal(rt, err_a->message, err_b->message);
}

static int error_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_error(rt, b)) {
        return 1; /* Errors are greater than non-errors */
    }
    
    xc_error_t *err_a = (xc_error_t *)a;
    xc_error_t *err_b = (xc_error_t *)b;
    
    /* Compare by error code first */
    if (err_a->code < err_b->code) return -1;
    if (err_a->code > err_b->code) return 1;
    
    /* If codes are equal, compare messages */
    return xc_compare(rt, err_a->message, err_b->message);
}

/* Type descriptor for error type */
static xc_type_t error_type = {
    .name = "error",
    .flags = XC_TYPE_PRIMITIVE,
    .mark = error_mark,
    .free = error_free,
    .equal = error_equal,
    .compare = error_compare
};

/* Register error type */
void xc_register_error_type(xc_runtime_t *rt) {
    XC_RUNTIME_EXT(rt)->error_type = &error_type;
}

/* Create error object */
xc_object_t *xc_error_create(xc_runtime_t *rt, xc_error_code_t code, const char *message) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_error_t *err = (xc_error_t *)xc_gc_alloc(rt, sizeof(xc_error_t), XC_TYPE_ERROR);
    if (!err) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)err)->type = XC_RUNTIME_EXT(rt)->error_type;
    
    err->code = code;
    err->message = message ? xc_string_create(rt, message) : NULL;
    err->stack_trace = NULL;
    err->cause = NULL;
    
    return (xc_object_t *)err;
}

/* Error operations */
xc_error_code_t xc_error_get_code(xc_runtime_t *rt, xc_object_t *error) {
    assert(xc_is_error(rt, error));
    xc_error_t *err = (xc_error_t *)error;
    return err->code;
}

const char *xc_error_get_message(xc_runtime_t *rt, xc_object_t *error) {
    assert(xc_is_error(rt, error));
    xc_error_t *err = (xc_error_t *)error;
    return err->message ? xc_string_value(rt, err->message) : "";
}

void xc_error_set_stack_trace(xc_runtime_t *rt, xc_object_t *error, xc_object_t *stack_trace) {
    assert(xc_is_error(rt, error));
    assert(!stack_trace || xc_is_array(rt, stack_trace));
    
    xc_error_t *err = (xc_error_t *)error;
    
    if (err->stack_trace) {
        xc_gc_free(rt, err->stack_trace);
    }
    
    err->stack_trace = stack_trace;
    if (stack_trace) {
        xc_gc_add_ref(rt, stack_trace);
    }
}

xc_object_t *xc_error_get_stack_trace(xc_runtime_t *rt, xc_object_t *error) {
    assert(xc_is_error(rt, error));
    xc_error_t *err = (xc_error_t *)error;
    return err->stack_trace;
}

void xc_error_set_cause(xc_runtime_t *rt, xc_object_t *error, xc_object_t *cause) {
    assert(xc_is_error(rt, error));
    assert(!cause || xc_is_error(rt, cause));
    
    xc_error_t *err = (xc_error_t *)error;
    
    if (err->cause) {
        xc_gc_free(rt, err->cause);
    }
    
    err->cause = cause;
    if (cause) {
        xc_gc_add_ref(rt, cause);
    }
}

xc_object_t *xc_error_get_cause(xc_runtime_t *rt, xc_object_t *error) {
    assert(xc_is_error(rt, error));
    xc_error_t *err = (xc_error_t *)error;
    return err->cause;
}

/* Type checking */
bool xc_is_error(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->error_type;
}
