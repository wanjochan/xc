/*
 * xc_number.c - Number type implementation
 */

#include "../xc.h"
#include "../xc_internal.h"
#include "../xc_gc.h"
#include "../xc_internal.h"

/* Number type structure */
typedef struct {
    xc_object_t base;  /* Must be first */
    double value;      /* F64 value */
} xc_number_t;

/* Number methods */
static void number_mark(xc_runtime_t *rt, xc_object_t *obj) {
    /* Numbers don't have references to other objects */
}

static void number_free(xc_runtime_t *rt, xc_object_t *obj) {
    /* Memory is managed by GC */
}

static bool number_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_number(rt, b)) {
        return false;
    }
    xc_number_t *num_a = (xc_number_t *)a;
    xc_number_t *num_b = (xc_number_t *)b;
    return num_a->value == num_b->value;
}

static int number_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_number(rt, b)) {
        return 1; /* Numbers are greater than non-numbers */
    }
    xc_number_t *num_a = (xc_number_t *)a;
    xc_number_t *num_b = (xc_number_t *)b;
    if (num_a->value < num_b->value) return -1;
    if (num_a->value > num_b->value) return 1;
    return 0;
}

/* Type descriptor for number type */
static xc_type_t number_type = {
    .name = "number",
    .flags = XC_TYPE_PRIMITIVE,
    .mark = number_mark,
    .free = number_free,
    .equal = number_equal,
    .compare = number_compare
};

/* Register number type */
void xc_register_number_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    static xc_type_lifecycle_t lifecycle = {
        .initializer = NULL,
        .cleaner = NULL,
        .creator = NULL,  /* Number has its own creation functions */
        .destroyer = (xc_destroy_func)number_free,
        .marker = (xc_marker_func)number_mark,
        .allocator = NULL
    };
    
    /* 注册类型 */
    int type_id = xc_register_type("number", &lifecycle);
    XC_RUNTIME_EXT(rt)->number_type = &number_type;
}

/* Create number object */
xc_object_t *xc_number_create(xc_runtime_t *rt, double value) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_number_t *obj = (xc_number_t *)xc_gc_alloc(rt, sizeof(xc_number_t), XC_TYPE_NUMBER);
    if (!obj) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)obj)->type = XC_RUNTIME_EXT(rt)->number_type;
    obj->value = value;
    return (xc_object_t *)obj;
}

/* Type checking */
bool xc_is_number(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->number_type;
}

/* Value access */
double xc_number_value(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_number(rt, obj));
    xc_number_t *num = (xc_number_t *)obj;
    return num->value;
}

/* Type conversion */
double xc_to_number(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) {
        return 0.0;
    }
    if (xc_is_number(rt, obj)) {
        return xc_number_value(rt, obj);
    }
    if (xc_is_boolean(rt, obj)) {
        return xc_boolean_value(rt, obj) ? 1.0 : 0.0;
    }
    if (xc_is_string(rt, obj)) {
        const char *str = xc_string_value(rt, obj);
        char *end;
        double val = strtod(str, &end);
        return *end == '\0' ? val : 0.0;
    }
    return 0.0;
}
