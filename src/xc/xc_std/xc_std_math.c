/*
 * xc_std_math.c - 数学标准库实现
 */

#include "../xc.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c
#include "../xc_internal.h"
/* Math库对象 */
static xc_val math_obj = NULL;

/* 数学函数实现 */

/* abs - 绝对值 */
static xc_val math_abs(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.abs requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, fabs(value));
}

/* max - 最大值 */
static xc_val math_max(xc_val self, va_list* args) {
    xc_val a = va_arg(*args, xc_val);
    xc_val b = va_arg(*args, xc_val);
    
    if (!xc.is(a, XC_TYPE_NUMBER) || !xc.is(b, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.max requires number arguments");
    }
    
    double value_a = *(double*)a;
    double value_b = *(double*)b;
    
    return xc.create(XC_TYPE_NUMBER, value_a > value_b ? value_a : value_b);
}

/* min - 最小值 */
static xc_val math_min(xc_val self, va_list* args) {
    xc_val a = va_arg(*args, xc_val);
    xc_val b = va_arg(*args, xc_val);
    
    if (!xc.is(a, XC_TYPE_NUMBER) || !xc.is(b, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.min requires number arguments");
    }
    
    double value_a = *(double*)a;
    double value_b = *(double*)b;
    
    return xc.create(XC_TYPE_NUMBER, value_a < value_b ? value_a : value_b);
}

/* round - 四舍五入 */
static xc_val math_round(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.round requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, round(value));
}

/* floor - 向下取整 */
static xc_val math_floor(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.floor requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, floor(value));
}

/* ceil - 向上取整 */
static xc_val math_ceil(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.ceil requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, ceil(value));
}

/* random - 随机数生成 */
static xc_val math_random(xc_val self, va_list* args) {
    /* 生成0到1之间的随机数 */
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    
    double random_value = (double)rand() / RAND_MAX;
    return xc.create(XC_TYPE_NUMBER, random_value);
}

/* pow - 幂运算 */
static xc_val math_pow(xc_val self, va_list* args) {
    xc_val base = va_arg(*args, xc_val);
    xc_val exponent = va_arg(*args, xc_val);
    
    if (!xc.is(base, XC_TYPE_NUMBER) || !xc.is(exponent, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.pow requires number arguments");
    }
    
    double base_val = *(double*)base;
    double exp_val = *(double*)exponent;
    
    return xc.create(XC_TYPE_NUMBER, pow(base_val, exp_val));
}

/* sqrt - 平方根 */
static xc_val math_sqrt(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_TYPE, "Math.sqrt requires a number argument");
    }
    
    double value = *(double*)num;
    if (value < 0) {
        return xc.create(XC_TYPE_EXCEPTION, XC_ERR_VALUE, "Math.sqrt cannot be called with negative numbers");
    }
    
    return xc.create(XC_TYPE_NUMBER, sqrt(value));
}

/* 创建Math对象 */
static xc_val create_math_object(void) {
    xc_val obj = xc.create(XC_TYPE_OBJECT);
    
    /* 添加常量 */
    xc_val pi_val = xc.create(XC_TYPE_NUMBER, M_PI);
    xc_object_set(&xc, obj, "PI", pi_val);
    xc_release(pi_val);
    
    xc_val e_val = xc.create(XC_TYPE_NUMBER, M_E);
    xc_object_set(&xc, obj, "E", e_val);
    xc_release(e_val);
    
    /* 添加函数 */
    xc_val abs_func = xc.create(XC_TYPE_FUNC, math_abs, NULL);
    xc_object_set(&xc, obj, "abs", abs_func);
    xc_release(abs_func);
    
    xc_val max_func = xc.create(XC_TYPE_FUNC, math_max, NULL);
    xc_object_set(&xc, obj, "max", max_func);
    xc_release(max_func);
    
    xc_val min_func = xc.create(XC_TYPE_FUNC, math_min, NULL);
    xc_object_set(&xc, obj, "min", min_func);
    xc_release(min_func);
    
    xc_val round_func = xc.create(XC_TYPE_FUNC, math_round, NULL);
    xc_object_set(&xc, obj, "round", round_func);
    xc_release(round_func);
    
    xc_val floor_func = xc.create(XC_TYPE_FUNC, math_floor, NULL);
    xc_object_set(&xc, obj, "floor", floor_func);
    xc_release(floor_func);
    
    xc_val ceil_func = xc.create(XC_TYPE_FUNC, math_ceil, NULL);
    xc_object_set(&xc, obj, "ceil", ceil_func);
    xc_release(ceil_func);
    
    xc_val random_func = xc.create(XC_TYPE_FUNC, math_random, NULL);
    xc_object_set(&xc, obj, "random", random_func);
    xc_release(random_func);
    
    xc_val pow_func = xc.create(XC_TYPE_FUNC, math_pow, NULL);
    xc_object_set(&xc, obj, "pow", pow_func);
    xc_release(pow_func);
    
    xc_val sqrt_func = xc.create(XC_TYPE_FUNC, math_sqrt, NULL);
    xc_object_set(&xc, obj, "sqrt", sqrt_func);
    xc_release(sqrt_func);
    
    return obj;
}

/* 获取Math对象 */
xc_val xc_std_get_math(void) {
    if (math_obj == NULL) {
        math_obj = create_math_object();
    }
    return math_obj;
}

/* 初始化Math库 */
void xc_std_math_initialize(void) {
    /* 创建全局Math对象 */
    math_obj = create_math_object();
    
    /* 注意：在当前版本中，我们不将Math对象添加到全局对象
     * 因为全局对象访问机制尚未实现
     * 用户需要通过xc_std_get_math()函数获取Math对象
     */
}

/* 清理Math库 */
void xc_std_math_cleanup(void) {
    if (math_obj != NULL) {
        xc_release(math_obj);
        math_obj = NULL;
    }
}

/* 使用构造函数自动初始化Math库 */
__attribute__((constructor)) static void register_math_lib(void) {
    printf("register_math_lib()\n");
} 
