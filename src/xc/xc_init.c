/*
 * xc_init.c - XC Runtime Type Initialization
 */

#include "xc.h"

/* 类型初始化声明 */
void xc_string_register(void);
void xc_boolean_register(void);
void xc_number_register(void);
void xc_array_register(void);
void xc_object_register(void);
void xc_function_register(void);

/* 按顺序初始化所有基本类型 */
void xc_types_init(void) {
    /* 注册基本类型 - 顺序很重要 */
    xc_string_register();    /* 字符串必须最先初始化 */
    xc_boolean_register();
    xc_number_register();
    xc_array_register();
    xc_object_register();
    xc_function_register();
}

/* 自动注册所有类型 */
static void __attribute__((constructor)) xc_auto_init(void) {
    xc_types_init();
}