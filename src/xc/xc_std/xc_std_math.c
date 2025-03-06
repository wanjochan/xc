/*
 * xc_std_math.c - 数学标准库实现
 */

#include "../xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Math库对象 */
static xc_val math_obj = NULL;

/* 数学函数实现 */

/* abs - 绝对值 */
static xc_val math_abs(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.error("Math.abs requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, fabs(value));
}

/* max - 最大值 */
static xc_val math_max(xc_val self, va_list* args) {
    xc_val a = va_arg(*args, xc_val);
    xc_val b = va_arg(*args, xc_val);
    
    if (!xc.is(a, XC_TYPE_NUMBER) || !xc.is(b, XC_TYPE_NUMBER)) {
        return xc.error("Math.max requires number arguments");
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
        return xc.error("Math.min requires number arguments");
    }
    
    double value_a = *(double*)a;
    double value_b = *(double*)b;
    
    return xc.create(XC_TYPE_NUMBER, value_a < value_b ? value_a : value_b);
}

/* round - 四舍五入 */
static xc_val math_round(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.error("Math.round requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, round(value));
}

/* floor - 向下取整 */
static xc_val math_floor(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.error("Math.floor requires a number argument");
    }
    
    double value = *(double*)num;
    return xc.create(XC_TYPE_NUMBER, floor(value));
}

/* ceil - 向上取整 */
static xc_val math_ceil(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.error("Math.ceil requires a number argument");
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
        return xc.error("Math.pow requires number arguments");
    }
    
    double base_val = *(double*)base;
    double exp_val = *(double*)exponent;
    
    return xc.create(XC_TYPE_NUMBER, pow(base_val, exp_val));
}

/* sqrt - 平方根 */
static xc_val math_sqrt(xc_val self, va_list* args) {
    xc_val num = va_arg(*args, xc_val);
    
    if (!xc.is(num, XC_TYPE_NUMBER)) {
        return xc.error("Math.sqrt requires a number argument");
    }
    
    double value = *(double*)num;
    if (value < 0) {
        return xc.error("Math.sqrt cannot be called with negative numbers");
    }
    
    return xc.create(XC_TYPE_NUMBER, sqrt(value));
}

/* 创建Math对象 */
static xc_val create_math_object(void) {
    xc_val obj = xc.create(XC_TYPE_OBJECT);
    
    /* 添加常量 */
    xc_object_set_property(obj, "PI", xc.create(XC_TYPE_NUMBER, M_PI));
    xc_object_set_property(obj, "E", xc.create(XC_TYPE_NUMBER, M_E));
    
    /* 添加函数 */
    xc_object_set_property(obj, "abs", xc.func(math_abs));
    xc_object_set_property(obj, "max", xc.func(math_max));
    xc_object_set_property(obj, "min", xc.func(math_min));
    xc_object_set_property(obj, "round", xc.func(math_round));
    xc_object_set_property(obj, "floor", xc.func(math_floor));
    xc_object_set_property(obj, "ceil", xc.func(math_ceil));
    xc_object_set_property(obj, "random", xc.func(math_random));
    xc_object_set_property(obj, "pow", xc.func(math_pow));
    xc_object_set_property(obj, "sqrt", xc.func(math_sqrt));
    
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
    
    /* 将Math对象添加到全局对象 */
    xc_val global = xc.get_global_object();
    xc_object_set_property(global, "Math", math_obj);
}

/* 清理Math库 */
void xc_std_math_cleanup(void) {
    if (math_obj != NULL) {
        xc.release(math_obj);
        math_obj = NULL;
    }
}

/* 使用构造函数自动初始化Math库 */
__attribute__((constructor)) static void register_math_lib(void) {
    printf("register_math_lib()\n");
} 