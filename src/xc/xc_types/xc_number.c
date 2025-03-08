#include "../xc.h"
#include "../xc_internal.h"

static xc_runtime_t* rt = NULL;

/* Forward declarations */
static void number_free(xc_runtime_t *rt, xc_object_t *obj);
static bool number_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static int number_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static xc_val number_creator(int type, va_list args);

/* Number type structure */
typedef struct {
    xc_object_t base;  /* Must be first */
    double value;      /* F64 value */
} xc_number_t;

/* Number methods */
static void number_mark(xc_object_t *obj, mark_func mark) {
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

/* 获取数字值 */
static void* number_get_value(xc_val obj) {
    xc_number_t* number = (xc_number_t*)obj;
    // 返回指向值的指针（注意：这里需要静态存储）
    static double value;
    value = number->value;
    return &value;
}

/* 转换到其他类型 */
static xc_val number_convert_to(xc_val obj, int target_type) {
    xc_number_t* number = (xc_number_t*)obj;
    double value = number->value;
    
    switch (target_type) {
        case XC_TYPE_BOOL:
            return rt->new(XC_TYPE_BOOL, value != 0.0);
            
        case XC_TYPE_NUMBER:
            return obj; // 已经是数字类型
            
        case XC_TYPE_STRING: {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%g", value);
            return rt->new(XC_TYPE_STRING, buffer);
        }
            
        default:
            return NULL; // 不支持的转换
    }
}

/* Type descriptor for number type */
static xc_type_lifecycle_t number_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = number_creator,
    .destroyer = (xc_destroy_func)number_free,
    .marker = number_mark,
    // .allocator = NULL,
    .name = "number",
    .equal = (bool (*)(xc_val, xc_val))number_equal,
    .compare = (int (*)(xc_val, xc_val))number_compare,
    .flags = XC_TYPE_PRIMITIVE,
    .get_value = number_get_value,
    .convert_to = number_convert_to
};

/* Number creator function for use with create() */
static xc_val number_creator(int type, va_list args) {
    /* 从可变参数中获取数值 */
    double value = va_arg(args, double);
    
    /* 调用实际的创建函数 */
    return (xc_val)xc_number_create(NULL, value);
}

/* Register number type */
void xc_register_number_type(xc_runtime_t *caller_rt) {
    rt = caller_rt;
    /* 定义类型生命周期管理接口 */
    /* 注册类型 */
    int type_id = rt->register_type("number", &number_type);
    // xc_number_type = &number_type;
}

/* Create a number object */
xc_object_t *xc_number_create(xc_runtime_t *rt, double value) {
    /* 分配内存 */
    xc_number_t *obj = (xc_number_t *)xc_gc_alloc(rt, sizeof(xc_number_t), XC_TYPE_NUMBER);
    if (!obj) {
        return NULL;
    }
    
    /* 初始化对象 */
    ((xc_object_t *)obj)->type_id = XC_TYPE_NUMBER;
    obj->value = value;
    
    return (xc_object_t *)obj;
}

/* Type checking */
bool xc_is_number(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type_id == XC_TYPE_NUMBER;
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
        if (!str) return 0.0;
        return atof(str);
    }
    
    return 0.0;
}
