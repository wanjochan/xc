/*
 * xc_null.c - Null type implementation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../xc.h"
#include "../xc_internal.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c

/* Forward declarations */
static void null_mark(xc_runtime_t *rt, xc_object_t *obj);
static void null_free(xc_runtime_t *rt, xc_object_t *obj);
static bool null_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static int null_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static xc_val null_creator(int type, va_list args);

/* Null object structure */
typedef struct {
    xc_object_t base;  /* Must be first */
} xc_null_t;

/* Singleton null instance */
static xc_object_t *null_singleton = NULL;

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
static xc_type_lifecycle_t null_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = null_creator,
    .destroyer = (xc_destroy_func)null_free,
    .marker = (xc_marker_func)null_mark,
    // .allocator = NULL,
    .name = "null",
    .equal = (bool (*)(xc_val, xc_val))null_equal,
    .compare = (int (*)(xc_val, xc_val))null_compare,
    .flags = XC_TYPE_PRIMITIVE
};

/* Null creator function for use with create() */
static xc_val null_creator(int type, va_list args) {
    /* 返回单例 null 对象 */
    return (xc_val)null_singleton;
}

/* Register null type */
void xc_register_null_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    /* 注册类型 */
    int type_id = xc_register_type("null", &null_type);
    xc_null_type = &null_type;

    /* Create singleton null instance if not already created */
    if (!null_singleton) {
        /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
        xc_null_t *obj = (xc_null_t *)xc_gc_alloc(rt, sizeof(xc_null_t), XC_TYPE_NULL);
        if (null_singleton) {
            /* 设置正确的类型ID */
            ((xc_object_t *)obj)->type_id = XC_TYPE_NULL;
            /* 使用 xc_gc_mark_permanent 标记为永久对象 */
            xc_gc_mark_permanent(rt, (xc_object_t *)obj);
            null_singleton = (xc_object_t *)obj;
        }
    }
}

/* Create null object - returns singleton instance */
xc_object_t *xc_null_create(xc_runtime_t *rt) {
    /* 如果单例已存在，直接返回 */
    if (null_singleton) {
        return null_singleton;
    }
    
    /* 分配内存 */
    xc_null_t *obj = (xc_null_t *)xc_gc_alloc(rt, sizeof(xc_null_t), XC_TYPE_NULL);
    if (!obj) {
        return NULL;
    }
    
    /* 初始化对象 */
    ((xc_object_t *)obj)->type_id = XC_TYPE_NULL;
    
    /* 保存单例 */
    null_singleton = (xc_object_t *)obj;
    
    return null_singleton;
}

/* Type checking */
bool xc_is_null(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type_id == XC_TYPE_NULL;
}