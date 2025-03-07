/*
 * xc_string.c - String type implementation
 */

#include "../xc.h"
#include "../xc_internal.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c

/* String object structure */
typedef struct {
    xc_object_t base;  /* Must be first */
    char *data;        /* String data */
    size_t length;     /* String length */
} xc_string_t;

/* String methods */
static void string_mark(xc_runtime_t *rt, xc_object_t *obj) {
    /* Strings don't have references to other objects */
}

static void string_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_string_t *str = (xc_string_t *)obj;
    if (str->data) {
        free(str->data);
        str->data = NULL;
    }
}

static bool string_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_string(rt, b)) {
        return false;
    }
    xc_string_t *str_a = (xc_string_t *)a;
    xc_string_t *str_b = (xc_string_t *)b;
    
    return str_a->length == str_b->length &&
           memcmp(str_a->data, str_b->data, str_a->length) == 0;
}

static int string_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_string(rt, b)) {
        return 1; /* Strings are greater than non-strings */
    }
    xc_string_t *str_a = (xc_string_t *)a;
    xc_string_t *str_b = (xc_string_t *)b;
    
    return strcmp(str_a->data, str_b->data);
}

/* Type descriptor for string type */
static xc_type_t string_type = {
    .name = "string",
    .flags = XC_TYPE_PRIMITIVE,
    .mark = string_mark,
    .free = string_free,
    .equal = string_equal,
    .compare = string_compare
};

/* Register string type */
void xc_register_string_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    static xc_type_lifecycle_t lifecycle = {
        .initializer = NULL,
        .cleaner = NULL,
        .creator = NULL,  /* String has its own creation functions */
        .destroyer = (xc_destroy_func)string_free,
        .marker = (xc_marker_func)string_mark,
        .allocator = NULL
    };
    
    /* 注册类型 */
    int type_id = xc_register_type("string", &lifecycle);
    XC_RUNTIME_EXT(rt)->string_type = &string_type;
}

/* Internal helper functions */
static xc_object_t *string_alloc(xc_runtime_t *rt, const char *str, size_t len) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_string_t *obj = (xc_string_t *)xc_gc_alloc(rt, sizeof(xc_string_t), XC_TYPE_STRING);
    if (!obj) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)obj)->type = XC_RUNTIME_EXT(rt)->string_type;

    obj->data = malloc(len + 1);
    if (!obj->data) {
        xc_gc_free(rt, (xc_object_t *)obj);
        return NULL;
    }

    if (str) {
        memcpy(obj->data, str, len);
    } else {
        memset(obj->data, 0, len);
    }
    obj->data[len] = '\0';
    obj->length = len;

    return (xc_object_t *)obj;
}

/* Create string object */
xc_object_t *xc_string_create(xc_runtime_t *rt, const char *value) {
    if (!value) {
        value = "";
    }
    return string_alloc(rt, value, strlen(value));
}

/* Create string object with length */
xc_object_t *xc_string_create_len(xc_runtime_t *rt, const char *value, size_t len) {
    return string_alloc(rt, value, len);
}

/* Type checking */
bool xc_is_string(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->string_type;
}

/* Value access */
const char *xc_string_value(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_string(rt, obj));
    xc_string_t *str = (xc_string_t *)obj;
    return str->data;
}

size_t xc_string_length(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_string(rt, obj));
    xc_string_t *str = (xc_string_t *)obj;
    return str->length;
}

/* Type conversion */
const char *xc_to_string(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) {
        return "null";
    }

    if (xc_is_string(rt, obj)) {
        return xc_string_value(rt, obj);
    }

    if (xc_is_boolean(rt, obj)) {
        return xc_boolean_value(rt, obj) ? "true" : "false";
    }

    if (xc_is_number(rt, obj)) {
        /* Convert number to string */
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.14g", xc_number_value(rt, obj));
        xc_object_t *str = xc_string_create(rt, buffer);
        if (!str) {
            return "error";
        }
        return xc_string_value(rt, str);
    }

    return "object";
}