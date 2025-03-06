/*
 * xc_boolean.c - Boolean type implementation
 */

#include "../xc.h"
#include "../xc_object.h"
#include "../xc_gc.h"
#include "xc_types.h"

/* Boolean object structure */
typedef struct {
    xc_object_t base;     /* Must be first */
    bool value;           /* Boolean value */
} xc_boolean_t;

/* Boolean methods */
static void boolean_mark(xc_runtime_t *rt, xc_object_t *obj) {
    /* Booleans don't have references to other objects */
}

static void boolean_free(xc_runtime_t *rt, xc_object_t *obj) {
    /* No extra resources to free */
}

static bool boolean_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_boolean(rt, b)) {
        return false;
    }
    
    xc_boolean_t *bool_a = (xc_boolean_t *)a;
    xc_boolean_t *bool_b = (xc_boolean_t *)b;
    
    return bool_a->value == bool_b->value;
}

static int boolean_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_boolean(rt, b)) {
        return 1; /* Booleans are greater than non-booleans */
    }
    
    xc_boolean_t *bool_a = (xc_boolean_t *)a;
    xc_boolean_t *bool_b = (xc_boolean_t *)b;
    
    if (bool_a->value == bool_b->value) return 0;
    return bool_a->value ? 1 : -1;  /* true > false */
}

/* Type descriptor for boolean type */
static xc_type_t boolean_type = {
    .name = "boolean",
    .flags = XC_TYPE_PRIMITIVE,
    .mark = boolean_mark,
    .free = boolean_free,
    .equal = boolean_equal,
    .compare = boolean_compare
};

/* Register boolean type */
void xc_register_boolean_type(xc_runtime_t *rt) {
    XC_RUNTIME_EXT(rt)->boolean_type = &boolean_type;
}

/* Create boolean object */
xc_object_t *xc_boolean_create(xc_runtime_t *rt, bool value) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_boolean_t *obj = (xc_boolean_t *)xc_gc_alloc(rt, sizeof(xc_boolean_t), XC_TYPE_BOOL);
    if (!obj) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)obj)->type = XC_RUNTIME_EXT(rt)->boolean_type;

    obj->value = value;
    return (xc_object_t *)obj;
}

/* Type checking */
bool xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->boolean_type;
}

/* Value access */
bool xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_boolean(rt, obj));
    xc_boolean_t *boolean = (xc_boolean_t *)obj;
    return boolean->value;
}

/* Type conversion */
bool xc_to_boolean(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) {
        return false;
    }

    if (xc_is_boolean(rt, obj)) {
        return xc_boolean_value(rt, obj);
    }

    if (xc_is_number(rt, obj)) {
        return xc_number_value(rt, obj) != 0.0;
    }

    if (xc_is_string(rt, obj)) {
        const char *str = xc_string_value(rt, obj);
        return str && str[0] != '\0';  /* Non-empty string is true */
    }

    if (xc_is_array(rt, obj)) {
        return xc_array_length(rt, obj) > 0;  /* Non-empty array is true */
    }

    return true;  /* All other objects are true */
}
