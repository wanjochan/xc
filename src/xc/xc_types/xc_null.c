/*
 * xc_null.c - Null type implementation
 */

#include "../xc.h"
#include "../xc_object.h"
#include "../xc_gc.h"
#include "../xc_runtime_internal.h"
#include "xc_types.h"

/* Null object structure */
typedef struct {
    xc_object_t base;  /* Must be first */
} xc_null_t;

/* Singleton null instance */
static xc_null_t *null_singleton = NULL;

/* Null methods */
static void null_mark(xc_runtime_t *rt, xc_object_t *obj) {
    /* Null doesn't have references to other objects */
}

static void null_free(xc_runtime_t *rt, xc_object_t *obj) {
    /* No resources to free */
}

static bool null_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    /* All null objects are equal */
    return xc_is_null(rt, b);
}

static int null_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (xc_is_null(rt, b)) {
        return 0;  /* Null equals null */
    }
    return -1;    /* Null is less than any other type */
}

/* Type descriptor for null type */
static xc_type_t null_type = {
    .name = "null",
    .flags = XC_TYPE_PRIMITIVE,
    .mark = null_mark,
    .free = null_free,
    .equal = null_equal,
    .compare = null_compare
};

/* Register null type */
void xc_register_null_type(xc_runtime_t *rt) {
    XC_RUNTIME_EXT(rt)->null_type = &null_type;

    /* Create singleton null instance if not already created */
    if (!null_singleton) {
        /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
        null_singleton = (xc_null_t *)xc_gc_alloc(rt, sizeof(xc_null_t), XC_TYPE_NULL);
        if (null_singleton) {
            /* 设置正确的类型指针 */
            ((xc_object_t *)null_singleton)->type = XC_RUNTIME_EXT(rt)->null_type;
            /* 使用 xc_gc_mark_permanent 标记为永久对象 */
            xc_gc_mark_permanent(rt, (xc_object_t *)null_singleton);
        }
    }
}

/* Create null object - returns singleton instance */
xc_object_t *xc_null_create(xc_runtime_t *rt) {
    assert(null_singleton != NULL);
    return (xc_object_t *)null_singleton;
}

/* Type checking */
bool xc_is_null(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->null_type;
}