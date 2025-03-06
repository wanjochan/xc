/*
 * test_xc_stdc.c - 标准库和复合数据类型测试
 */

#include "xc.h"
#include "xc_std_math.h"
#include "xc_std_console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 测试Array类型方法 */
static void test_array_methods(void) {
    printf("\n=== 测试Array类型方法 ===\n");
    
    /* 创建测试数组 */
    xc_val arr = xc.create(XC_TYPE_ARRAY, 0);
    
    /* 测试push和length */
    xc.call(arr, "push", xc.create(XC_TYPE_NUMBER, 10.0));
    xc.call(arr, "push", xc.create(XC_TYPE_NUMBER, 20.0));
    xc.call(arr, "push", xc.create(XC_TYPE_NUMBER, 30.0));
    
    xc_val length = xc.call(arr, "length");
    printf("数组长度: %d\n", (int)*(double*)length);
    assert(*(double*)length == 3.0);
    xc.release(length);
    
    /* 测试get和set */
    xc_val index1 = xc.create(XC_TYPE_NUMBER, 1.0);
    xc_val value = xc.call(arr, "get", index1);
    printf("arr[1] = %f\n", *(double*)value);
    assert(*(double*)value == 20.0);
    xc.release(value);
    
    xc_val new_value = xc.create(XC_TYPE_NUMBER, 25.0);
    xc.call(arr, "set", index1, new_value);
    xc.release(new_value);
    xc.release(index1);
    
    index1 = xc.create(XC_TYPE_NUMBER, 1.0);
    value = xc.call(arr, "get", index1);
    printf("修改后 arr[1] = %f\n", *(double*)value);
    assert(*(double*)value == 25.0);
    xc.release(value);
    xc.release(index1);
    
    /* 测试indexOf */
    xc_val item = xc.create(XC_TYPE_NUMBER, 30.0);
    xc_val index = xc.call(arr, "indexOf", item);
    printf("30的索引: %d\n", (int)*(double*)index);
    assert(*(double*)index == 2.0);
    xc.release(index);
    
    xc_val not_found = xc.create(XC_TYPE_NUMBER, 50.0);
    index = xc.call(arr, "indexOf", not_found);
    printf("50的索引: %d\n", (int)*(double*)index);
    assert(*(double*)index == -1.0);
    xc.release(index);
    xc.release(not_found);
    xc.release(item);
    
    /* 测试slice */
    xc_val start = xc.create(XC_TYPE_NUMBER, 0.0);
    xc_val end = xc.create(XC_TYPE_NUMBER, 2.0);
    xc_val slice = xc.call(arr, "slice", start, end);
    xc_val slice_str = xc.call(slice, "toString");
    printf("slice(0,2): %s\n", (char*)slice_str);
    xc.release(slice_str);
    
    length = xc.call(slice, "length");
    assert(*(double*)length == 2.0);
    xc.release(length);
    xc.release(slice);
    xc.release(start);
    xc.release(end);
    
    /* 测试join */
    xc_val separator = xc.create(XC_TYPE_STRING, " - ");
    xc_val joined = xc.call(arr, "join", separator);
    printf("join结果: %s\n", (char*)joined);
    xc.release(joined);
    xc.release(separator);
    
    /* 测试forEach */
    printf("forEach测试:\n");
    xc_val forEach_func = xc.create(XC_TYPE_FUNC, 
        (xc_func_ptr)function(xc_val self, int argc, xc_val* argv, xc_val closure) {
            printf("  元素: %f, 索引: %d\n", 
                   *(double*)argv[0], 
                   (int)*(double*)argv[1]);
            return NULL;
        }, 3, NULL);
    
    xc.call(arr, "forEach", forEach_func);
    xc.release(forEach_func);
    
    /* 测试map */
    xc_val map_func = xc.create(XC_TYPE_FUNC, 
        (xc_func_ptr)function(xc_val self, int argc, xc_val* argv, xc_val closure) {
            double value = *(double*)argv[0] * 2;
            return xc.create(XC_TYPE_NUMBER, value);
        }, 3, NULL);
    
    xc_val mapped = xc.call(arr, "map", map_func);
    xc_val mapped_str = xc.call(mapped, "toString");
    printf("map结果: %s\n", (char*)mapped_str);
    xc.release(mapped_str);
    xc.release(mapped);
    xc.release(map_func);
    
    /* 测试filter */
    xc_val filter_func = xc.create(XC_TYPE_FUNC, 
        (xc_func_ptr)function(xc_val self, int argc, xc_val* argv, xc_val closure) {
            double value = *(double*)argv[0];
            return xc.create(XC_TYPE_BOOLEAN, value > 20.0);
        }, 3, NULL);
    
    xc_val filtered = xc.call(arr, "filter", filter_func);
    xc_val filtered_str = xc.call(filtered, "toString");
    printf("filter结果: %s\n", (char*)filtered_str);
    xc.release(filtered_str);
    xc.release(filtered);
    xc.release(filter_func);
    
    /* 清理 */
    xc.release(arr);
    
    printf("Array类型方法测试通过!\n");
}

/* 测试String类型方法 */
static void test_string_methods(void) {
    printf("\n=== 测试String类型方法 ===\n");
    
    /* 创建测试字符串 */
    xc_val str = xc.create(XC_TYPE_STRING, "  Hello, World!  ");
    
    /* 测试length */
    xc_val length = xc.call(str, "length");
    printf("字符串长度: %d\n", (int)*(double*)length);
    assert(*(double*)length == 17.0);
    xc.release(length);
    
    /* 测试indexOf */
    xc_val search = xc.create(XC_TYPE_STRING, "World");
    xc_val index = xc.call(str, "indexOf", search);
    printf("'World'的索引: %d\n", (int)*(double*)index);
    assert(*(double*)index == 9.0);
    xc.release(index);
    xc.release(search);
    
    /* 测试substring */
    xc_val start = xc.create(XC_TYPE_NUMBER, 2.0);
    xc_val end = xc.create(XC_TYPE_NUMBER, 7.0);
    xc_val substr = xc.call(str, "substring", start, end);
    printf("substring(2,7): %s\n", (char*)substr);
    assert(strcmp((char*)substr, "Hello") == 0);
    xc.release(substr);
    xc.release(start);
    xc.release(end);
    
    /* 测试split */
    xc_val separator = xc.create(XC_TYPE_STRING, ", ");
    xc_val parts = xc.call(str, "split", separator);
    xc_val parts_str = xc.call(parts, "toString");
    printf("split结果: %s\n", (char*)parts_str);
    xc.release(parts_str);
    
    length = xc.call(parts, "length");
    assert(*(double*)length == 2.0);
    xc.release(length);
    xc.release(parts);
    xc.release(separator);
    
    /* 测试trim */
    xc_val trimmed = xc.call(str, "trim");
    printf("trim结果: '%s'\n", (char*)trimmed);
    assert(strcmp((char*)trimmed, "Hello, World!") == 0);
    xc.release(trimmed);
    
    /* 测试toLowerCase */
    xc_val lower = xc.call(str, "toLowerCase");
    printf("toLowerCase结果: %s\n", (char*)lower);
    assert(strstr((char*)lower, "hello") != NULL);
    xc.release(lower);
    
    /* 测试toUpperCase */
    xc_val upper = xc.call(str, "toUpperCase");
    printf("toUpperCase结果: %s\n", (char*)upper);
    assert(strstr((char*)upper, "HELLO") != NULL);
    xc.release(upper);
    
    /* 清理 */
    xc.release(str);
    
    printf("String类型方法测试通过!\n");
}

/* 测试Math库 */
static void test_math_lib(void) {
    printf("\n=== 测试Math库 ===\n");
    
    /* 获取Math对象 */
    xc_val math = xc_std_get_math();
    
    /* 测试abs */
    xc_val neg = xc.create(XC_TYPE_NUMBER, -42.5);
    xc_val abs_val = xc.call(math, "abs", neg);
    printf("abs(-42.5) = %f\n", *(double*)abs_val);
    assert(*(double*)abs_val == 42.5);
    xc.release(abs_val);
    xc.release(neg);
    
    /* 测试max */
    xc_val a = xc.create(XC_TYPE_NUMBER, 10.0);
    xc_val b = xc.create(XC_TYPE_NUMBER, 20.0);
    xc_val max_val = xc.call(math, "max", a, b);
    printf("max(10, 20) = %f\n", *(double*)max_val);
    assert(*(double*)max_val == 20.0);
    xc.release(max_val);
    
    /* 测试min */
    xc_val min_val = xc.call(math, "min", a, b);
    printf("min(10, 20) = %f\n", *(double*)min_val);
    assert(*(double*)min_val == 10.0);
    xc.release(min_val);
    xc.release(a);
    xc.release(b);
    
    /* 测试round */
    xc_val frac = xc.create(XC_TYPE_NUMBER, 3.7);
    xc_val round_val = xc.call(math, "round", frac);
    printf("round(3.7) = %f\n", *(double*)round_val);
    assert(*(double*)round_val == 4.0);
    xc.release(round_val);
    xc.release(frac);
    
    /* 测试floor */
    frac = xc.create(XC_TYPE_NUMBER, 3.7);
    xc_val floor_val = xc.call(math, "floor", frac);
    printf("floor(3.7) = %f\n", *(double*)floor_val);
    assert(*(double*)floor_val == 3.0);
    xc.release(floor_val);
    xc.release(frac);
    
    /* 测试ceil */
    frac = xc.create(XC_TYPE_NUMBER, 3.2);
    xc_val ceil_val = xc.call(math, "ceil", frac);
    printf("ceil(3.2) = %f\n", *(double*)ceil_val);
    assert(*(double*)ceil_val == 4.0);
    xc.release(ceil_val);
    xc.release(frac);
    
    /* 测试random */
    printf("测试random生成10个随机数:\n");
    for (int i = 0; i < 10; i++) {
        xc_val rand_val = xc.call(math, "random");
        double rand_num = *(double*)rand_val;
        printf("  random() = %f\n", rand_num);
        assert(rand_num >= 0.0 && rand_num < 1.0);
        xc.release(rand_val);
    }
    
    /* 测试pow */
    a = xc.create(XC_TYPE_NUMBER, 2.0);
    b = xc.create(XC_TYPE_NUMBER, 3.0);
    xc_val pow_val = xc.call(math, "pow", a, b);
    printf("pow(2, 3) = %f\n", *(double*)pow_val);
    assert(*(double*)pow_val == 8.0);
    xc.release(pow_val);
    xc.release(a);
    xc.release(b);
    
    /* 测试sqrt */
    xc_val num = xc.create(XC_TYPE_NUMBER, 16.0);
    xc_val sqrt_val = xc.call(math, "sqrt", num);
    printf("sqrt(16) = %f\n", *(double*)sqrt_val);
    assert(*(double*)sqrt_val == 4.0);
    xc.release(sqrt_val);
    xc.release(num);
    
    printf("Math库测试通过!\n");
}

/* 测试Console库 */
static void test_console_lib(void) {
    printf("\n=== 测试Console库 ===\n");
    
    /* 获取Console对象 */
    xc_val console = xc_std_get_console();
    
    /* 测试log */
    xc_val str1 = xc.create(XC_TYPE_STRING, "Hello");
    xc_val str2 = xc.create(XC_TYPE_STRING, "World");
    xc_val num = xc.create(XC_TYPE_NUMBER, 42.0);
    
    printf("测试console.log:\n");
    xc.call(console, "log", str1, str2, num);
    
    /* 测试error */
    printf("测试console.error:\n");
    xc.call(console, "error", xc.create(XC_TYPE_STRING, "这是一个错误消息"));
    
    /* 测试warn */
    printf("测试console.warn:\n");
    xc.call(console, "warn", xc.create(XC_TYPE_STRING, "这是一个警告消息"));
    
    /* 测试time/timeEnd */
    xc_val label = xc.create(XC_TYPE_STRING, "测试计时器");
    printf("测试console.time/timeEnd:\n");
    xc.call(console, "time", label);
    
    /* 模拟一些工作 */
    for (int i = 0; i < 1000000; i++) {
        /* 浪费一些CPU时间 */
    }
    
    xc.call(console, "timeEnd", label);
    
    /* 清理 */
    xc.release(label);
    xc.release(str1);
    xc.release(str2);
    xc.release(num);
    
    printf("Console库测试通过!\n");
}

/* 主测试函数 */
int main(void) {
    printf("=== 开始测试XC标准库和复合数据类型 ===\n");
    
    /* 初始化XC运行时 */
    xc_initialize();
    
    /* 初始化标准库 */
    xc_std_math_initialize();
    xc_std_console_initialize();
    
    /* 运行测试 */
    test_array_methods();
    test_string_methods();
    test_math_lib();
    test_console_lib();
    
    /* 清理标准库 */
    xc_std_console_cleanup();
    xc_std_math_cleanup();
    
    /* 关闭XC运行时 */
    xc_cleanup();
    
    printf("\n=== 所有测试通过! ===\n");
    return 0;
} 