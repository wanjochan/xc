/*
 * xc_boolean.c - Boolean type implementation
 */

#include "../xc.h"
#include "../xc_internal.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c
// Removed: xc_runtime_internal.h is now part of xc_internal.h
#include "../xc_internal.h"

/* Forward declarations */
static void boolean_mark(xc_runtime_t *rt, xc_object_t *obj);
static void boolean_free(xc_runtime_t *rt, xc_object_t *obj);
static bool boolean_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static int boolean_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static xc_val boolean_creator(int type, va_list args);

/* Boolean object structure */
typedef struct {
    xc_object_t base;     /* Must be first */
    bool value;           /* Boolean value */
} xc_boolean_t;

/* Singleton instances */
static xc_object_t *true_singleton = NULL;
static xc_object_t *false_singleton = NULL;

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
static xc_type_lifecycle_t boolean_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = boolean_creator,
    .destroyer = (xc_destroy_func)boolean_free,
    .marker = (xc_marker_func)boolean_mark,
    // .allocator = NULL,
    .name = "boolean",
    .equal = (bool (*)(xc_val, xc_val))boolean_equal,
    .compare = (int (*)(xc_val, xc_val))boolean_compare,
    .flags = XC_TYPE_PRIMITIVE
};

/* Boolean creator function for use with create() */
static xc_val boolean_creator(int type, va_list args) {
    /* 从可变参数中获取布尔值 */
    bool value = va_arg(args, int); /* bool在可变参数中被提升为int */
    
    /* 调用实际的创建函数 */
    return (xc_val)xc_boolean_create(NULL, value);
}

/* Register boolean type */
void xc_register_boolean_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    /* 注册类型 */
    int type_id = xc_register_type("boolean", &boolean_type);
    xc_boolean_type = &boolean_type;
}

/* Create a boolean object */
xc_object_t *xc_boolean_create(xc_runtime_t *rt, bool value) {
    /* 如果单例已存在，直接返回 */
    if (value && true_singleton) {
        return true_singleton;
    } else if (!value && false_singleton) {
        return false_singleton;
    }
    
    /* 分配内存 */
    xc_boolean_t *obj = (xc_boolean_t *)xc_gc_alloc(rt, sizeof(xc_boolean_t), XC_TYPE_BOOL);
    if (!obj) {
        return NULL;
    }
    
    /* 初始化对象 */
    ((xc_object_t *)obj)->type_id = XC_TYPE_BOOL;
    obj->value = value;
    
    /* 保存单例 */
    if (value) {
        true_singleton = (xc_object_t *)obj;
    } else {
        false_singleton = (xc_object_t *)obj;
    }
    
    return (xc_object_t *)obj;
}

/* Type checking */
bool xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type_id == XC_TYPE_BOOL;
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
