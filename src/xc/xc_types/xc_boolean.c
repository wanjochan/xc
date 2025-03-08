#include "../xc.h"

// /* Forward declarations */
// // static void boolean_mark(xc_runtime_t *rt, xc_object_t *obj);
// static void boolean_free(xc_runtime_t *rt, xc_object_t *obj);
// static bool boolean_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
// static int boolean_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
// static xc_val boolean_creator(int type, va_list args);

/* 声明需要使用的外部函数 */
extern double xc_to_number(xc_runtime_t *rt, xc_object_t *obj);

/* Boolean object structure */
typedef struct {
    xc_object_t base;     /* Must be first */
    bool value;           /* Boolean value */
} xc_boolean_t;

/* Singleton instances */
static xc_object_t *true_singleton = NULL;
static xc_object_t *false_singleton = NULL;

/* Boolean methods */
static void boolean_mark(xc_object_t *obj, mark_func mark) {
    /* Booleans don't have references to other objects */
}

static void boolean_free(xc_runtime_t *rt, xc_object_t *obj) {
    /* No extra resources to free */
}

static bool boolean_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!rt->is(b, XC_TYPE_BOOL)) {
        return false;
    }
    
    xc_boolean_t *bool_a = (xc_boolean_t *)a;
    xc_boolean_t *bool_b = (xc_boolean_t *)b;
    
    return bool_a->value == bool_b->value;
}

static int boolean_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!rt->is(b, XC_TYPE_BOOL)) {
        return 1; /* Booleans are greater than non-booleans */
    }
    
    xc_boolean_t *bool_a = (xc_boolean_t *)a;
    xc_boolean_t *bool_b = (xc_boolean_t *)b;
    
    if (bool_a->value == bool_b->value) return 0;
    return bool_a->value ? 1 : -1;  /* true > false */
}

/* Boolean creator function for use with create() */
static xc_val boolean_creator(int type, va_list args) {
    /* 从可变参数中获取布尔值 */
    bool value = va_arg(args, int); /* bool在可变参数中被提升为int */
    
    /* 调用实际的创建函数 */
    return (xc_val)xc.new(XC_TYPE_BOOL, value);
}

/* 获取布尔值 */
static void* boolean_get_value(xc_val obj) {
    xc_boolean_t* boolean = (xc_boolean_t*)obj;
    // 返回指向值的指针（注意：这里需要静态存储）
    static bool value;
    value = boolean->value;
    return &value;
}

/* 转换到其他类型 */
static xc_val boolean_convert_to(xc_val obj, int target_type) {
    xc_boolean_t* boolean = (xc_boolean_t*)obj;
    bool value = boolean->value;
    
    switch (target_type) {
        case XC_TYPE_BOOL:
            return obj; // 已经是布尔类型
            
        case XC_TYPE_NUMBER:
            return xc.new(XC_TYPE_NUMBER, value ? 1.0 : 0.0);
            
        case XC_TYPE_STRING:
            return xc.new(XC_TYPE_STRING, value ? "true" : "false");
            
        default:
            return NULL; // 不支持的转换
    }
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
    xc_boolean_t *obj = (xc_boolean_t *)xc.alloc(XC_TYPE_BOOL, sizeof(xc_boolean_t));
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

/* Type checking *///to remove later
bool xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type_id == XC_TYPE_BOOL;
}

/* Value access */
bool xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc.is(obj, XC_TYPE_BOOL));
    xc_boolean_t *boolean = (xc_boolean_t *)obj;
    return boolean->value;
}

/* Type conversion */
bool xc_to_boolean(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) {
        return false;
    }

    if (rt->is(obj, XC_TYPE_BOOL)) {
        // 直接获取布尔值
        void* value_ptr = rt->get_type_value(obj);
        if (value_ptr) {
            return *(bool*)value_ptr;
        }
        return false;
    }

    // 转换到布尔类型
    xc_val bool_obj = rt->convert_type(obj, XC_TYPE_BOOL);
    if (bool_obj) {
        void* value_ptr = rt->get_type_value(bool_obj);
        if (value_ptr) {
            return *(bool*)value_ptr;
        }
    }

    // 默认转换规则
    return obj != NULL; // 非NULL对象默认为true
}


/* Type descriptor for boolean type */
static xc_type_lifecycle_t boolean_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = boolean_creator,
    .destroyer = (xc_destroy_func)boolean_free,
    .marker = boolean_mark,
    // .allocator = NULL,
    .name = "boolean",
    .equal = (bool (*)(xc_val, xc_val))boolean_equal,
    .compare = (int (*)(xc_val, xc_val))boolean_compare,
    .flags = XC_TYPE_PRIMITIVE
};


/* Register boolean type */
void xc_register_boolean_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    boolean_type.initializer = NULL;
    boolean_type.cleaner = NULL;
    boolean_type.creator = boolean_creator;
    boolean_type.destroyer = (xc_destroy_func)boolean_free;
    boolean_type.marker = (xc_marker_func)boolean_mark;
    boolean_type.name = "boolean";
    boolean_type.equal = (void*)boolean_equal;
    boolean_type.compare = (void*)boolean_compare;
    
    // 新增：值访问和类型转换
    boolean_type.get_value = boolean_get_value;
    boolean_type.convert_to = boolean_convert_to;
    
    /* 注册类型 */
    int type_id = xc.register_type("boolean", &boolean_type);
    // xc_boolean_type = &boolean_type;
}