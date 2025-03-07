/*
 * xc_function.c - Function type implementation
 */

#include "../xc.h"
#include "../xc_internal.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c
#include "../xc_internal.h"

/* Function methods */
static void function_mark(xc_runtime_t *rt, xc_object_t *obj) {
    xc_function_t *func = (xc_function_t *)obj;
    if (func->this_obj) {
        /* 使用 xc_gc_mark 标记对象 */
        xc_gc_mark(rt, func->this_obj);
    }
    if (func->closure) {
        /* 标记闭包环境 */
        xc_gc_mark(rt, func->closure);
    }
}

static void function_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_function_t *func = (xc_function_t *)obj;
    if (func->this_obj) {
        /* 使用 xc_gc_free 释放对象 */
        xc_gc_free(rt, func->this_obj);
    }
    if (func->closure) {
        /* 释放闭包环境 */
        xc_gc_free(rt, func->closure);
    }
}

static bool function_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_function(rt, b)) {
        return false;
    }
    
    xc_function_t *func_a = (xc_function_t *)a;
    xc_function_t *func_b = (xc_function_t *)b;
    
    /* Functions are equal only if they have the same handler, closure and this_obj */
    return func_a->handler == func_b->handler && 
           func_a->closure == func_b->closure &&
           func_a->this_obj == func_b->this_obj;
}

static int function_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_function(rt, b)) {
        return 1; /* Functions are greater than non-functions */
    }
    
    xc_function_t *func_a = (xc_function_t *)a;
    xc_function_t *func_b = (xc_function_t *)b;
    
    /* Compare function pointers */
    if (func_a->handler < func_b->handler) return -1;
    if (func_a->handler > func_b->handler) return 1;
    
    /* If handlers are equal, compare closures (pointer comparison) */
    if (func_a->closure < func_b->closure) return -1;
    if (func_a->closure > func_b->closure) return 1;
    
    return 0;
}

/* Type descriptor for function type */
static xc_type_t function_type = {
    .name = "function",
    .flags = XC_TYPE_CALLABLE,
    .mark = function_mark,
    .free = function_free,
    .equal = function_equal,
    .compare = function_compare
};

/* Register function type */
void xc_register_function_type(xc_runtime_t *rt) {
    /* 定义类型生命周期管理接口 */
    static xc_type_lifecycle_t lifecycle = {
        .initializer = NULL,
        .cleaner = NULL,
        .creator = NULL,  /* Function has its own creation functions */
        .destroyer = (xc_destroy_func)function_free,
        .marker = (xc_marker_func)function_mark,
        .allocator = NULL
    };
    
    /* 注册类型 */
    int type_id = xc_register_type("function", &lifecycle);
    XC_RUNTIME_EXT(rt)->function_type = &function_type;
}

/* Create function object */
xc_object_t *xc_function_create(xc_runtime_t *rt, xc_function_ptr_t fn, xc_object_t *closure) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_function_t *obj = (xc_function_t *)xc_gc_alloc(rt, sizeof(xc_function_t), XC_TYPE_FUNC);
    if (!obj) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)obj)->type = XC_RUNTIME_EXT(rt)->function_type;

    obj->handler = fn;
    obj->this_obj = NULL;
    obj->closure = closure;  /* Initialize closure with the provided value */

    return (xc_object_t *)obj;
}

/* Function operations */
xc_object_t *xc_function_bind(xc_runtime_t *rt, xc_object_t *func, xc_object_t *this_obj) {
    assert(xc_is_function(rt, func));
    xc_function_t *function = (xc_function_t *)func;
    
    if (function->this_obj) {
        /* 使用 xc_gc_free 释放对象 */
        xc_gc_free(rt, function->this_obj);
    }
    
    function->this_obj = this_obj;
    if (this_obj) {
        /* 使用 xc_gc_add_ref 增加引用计数 */
        xc_gc_add_ref(rt, this_obj);
    }
    
    return func;
}

xc_object_t *xc_function_call(xc_runtime_t *rt, xc_object_t *func, xc_object_t *this_obj, size_t argc, xc_object_t **argv) {
    assert(xc_is_function(rt, func));
    xc_function_t *function = (xc_function_t *)func;
    
    /* Use bound this_obj if no this_obj provided */
    if (!this_obj) {
        this_obj = function->this_obj;
    }
    
    if (function->handler) {
        /* 类型转换以匹配函数签名 */
        return function->handler(rt, this_obj, argc, (void **)argv);
    }
    
    return NULL;
}

/* Type checking */
bool xc_is_function(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->function_type;
}

/* Access closure */
xc_object_t *xc_function_get_closure(xc_runtime_t *rt, xc_object_t *func) {
    assert(xc_is_function(rt, func));
    xc_function_t *function = (xc_function_t *)func;
    return function->closure;
}

xc_object_t *xc_function_get_this(xc_runtime_t *rt, xc_object_t *func) {
    assert(xc_is_function(rt, func));
    xc_function_t *function = (xc_function_t *)func;
    return function->this_obj;
}