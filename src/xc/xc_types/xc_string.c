/*
 * xc_string.c - String type implementation
 */

#include "../xc.h"
#include "../xc_internal.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c

/* Forward declarations */
static void string_mark(xc_runtime_t *rt, xc_object_t *obj);
static void string_free(xc_runtime_t *rt, xc_object_t *obj);
static bool string_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static int string_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static xc_val string_creator(int type, va_list args);

/* String method implementations */
static xc_val string_concat_method(xc_val obj, xc_val arg);
static xc_val string_length_method(xc_val obj, xc_val arg);

/* String object structure */
typedef struct {
    xc_object_t base;  /* Must be first */
    size_t length;     /* String length */
    char data[];       /* Flexible array member for string data */
} xc_string_t;

/* String methods */
static void string_mark(xc_runtime_t *rt, xc_object_t *obj) {
    /* Strings don't have references to other objects */
}

static void string_free(xc_runtime_t *rt, xc_object_t *obj) {
    /* 不需要额外的清理，因为字符串数据是直接跟在对象后面的 */
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

/* String creator function for use with create() */
static xc_val string_creator(int type, va_list args) {
    const char *str = va_arg(args, const char *);
    return (xc_val)xc_string_create(NULL, str);
}

/* Type descriptor for string type */
static xc_type_lifecycle_t string_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = string_creator,
    .destroyer = (xc_destroy_func)string_free,
    .marker = (xc_marker_func)string_mark,
    .allocator = NULL,
    .name = "string",
    .equal = (bool (*)(xc_val, xc_val))string_equal,
    .compare = (int (*)(xc_val, xc_val))string_compare,
    .flags = XC_TYPE_PRIMITIVE
};

/* Register string type */
void xc_register_string_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    /* 注册类型 */
    int type_id = xc_register_type("string", &string_type);
    xc_string_type = &string_type;
    
    /* 注册字符串方法 */
    rt->register_method(XC_TYPE_STRING, "concat", string_concat_method);
    rt->register_method(XC_TYPE_STRING, "length", string_length_method);
}

/* Internal helper functions */
static xc_object_t *string_alloc(xc_runtime_t *rt, size_t len) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_string_t *obj = (xc_string_t *)xc_gc_alloc(rt, sizeof(xc_string_t) + len + 1, XC_TYPE_STRING);
    if (!obj) {
        return NULL;
    }
    
    /* 初始化对象 */
    ((xc_object_t *)obj)->type_id = XC_TYPE_STRING;
    obj->length = len;
    
    return (xc_object_t *)obj;
}

/* Create string object with specified length */
xc_object_t *xc_string_create_len(xc_runtime_t *rt, const char *value, size_t len) {
    xc_object_t *obj = string_alloc(rt, len);
    if (!obj) {
        return NULL;
    }
    
    xc_string_t *str = (xc_string_t *)obj;
    if (value) {
        memcpy(str->data, value, len);
    } else {
        memset(str->data, 0, len);
    }
    str->data[len] = '\0';
    
    return obj;
}

/* Create string object */
xc_object_t *xc_string_create(xc_runtime_t *rt, const char *value) {
    return xc_string_create_len(rt, value, value ? strlen(value) : 0);
}

/* Type checking */
bool xc_is_string(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type_id == XC_TYPE_STRING;
}

/* Value access */
const char *xc_string_value(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_string(rt, obj));
    xc_string_t *str = (xc_string_t *)obj;
    return str->data;
}

/* Length access */
size_t xc_string_length(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_string(rt, obj));
    xc_string_t *str = (xc_string_t *)obj;
    return str->length;
}

/* String operations */
xc_object_t *xc_string_concat(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    assert(xc_is_string(rt, a));
    
    const char *str_a = xc_string_value(rt, a);
    size_t len_a = xc_string_length(rt, a);
    
    /* Convert b to string if needed */
    char temp[64];
    const char *str_b;
    size_t len_b;
    
    if (xc_is_string(rt, b)) {
        str_b = xc_string_value(rt, b);
        len_b = xc_string_length(rt, b);
    } else if (xc_is_number(rt, b)) {
        double num = xc_number_value(rt, b);
        snprintf(temp, sizeof(temp), "%g", num);
        str_b = temp;
        len_b = strlen(temp);
    } else if (xc_is_boolean(rt, b)) {
        bool val = xc_boolean_value(rt, b);
        str_b = val ? "true" : "false";
        len_b = val ? 4 : 5;
    } else if (xc_is_null(rt, b)) {
        str_b = "null";
        len_b = 4;
    } else {
        str_b = "[object]";
        len_b = 8;
    }
    
    /* Allocate new string with combined length */
    xc_string_t *result = (xc_string_t *)string_alloc(rt, len_a + len_b);
    if (!result) {
        return NULL;
    }
    
    /* Copy both strings */
    memcpy(result->data, str_a, len_a);
    memcpy(result->data + len_a, str_b, len_b);
    result->data[len_a + len_b] = '\0';
    
    return (xc_object_t *)result;
}

/* String method implementations */
static xc_val string_concat_method(xc_val obj, xc_val arg) {
    return (xc_val)xc_string_concat(NULL, (xc_object_t *)obj, (xc_object_t *)arg);
}

static xc_val string_length_method(xc_val obj, xc_val arg) {
    size_t len = xc_string_length(NULL, (xc_object_t *)obj);
    return (xc_val)xc_number_create(NULL, (double)len);
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