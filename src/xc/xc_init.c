/*
 * xc_init.c - XC Runtime Type Initialization
 */

#include "xc.h"
#include "xc_gc.h"

/* 类型初始化声明 */
void xc_register_string_type(xc_runtime_t *rt);
void xc_register_boolean_type(xc_runtime_t *rt);
void xc_register_number_type(xc_runtime_t *rt);
void xc_register_array_type(xc_runtime_t *rt);
void xc_register_object_type(xc_runtime_t *rt);
void xc_register_function_type(xc_runtime_t *rt);

/* 按顺序初始化所有基本类型 */
void xc_types_init(void) {
    /* 注册基本类型 - 顺序很重要 */
    xc_runtime_t *rt = &xc;
    xc_register_string_type(rt);    /* 字符串必须最先初始化 */
    xc_register_boolean_type(rt);
    xc_register_number_type(rt);
    xc_register_array_type(rt);
    xc_register_object_type(rt);
    xc_register_function_type(rt);
}

/* use GCC FEATURE to auto register all types */
void __attribute__((constructor)) xc_auto_init(void) {
    printf("\nTMP xc_auto_init()\n");
    xc_types_init();
}

/* 初始化XC运行时 */
void xc_init(void) {
    xc_types_init();
}

void __attribute__((destructor)) xc_shutdown(void) {
    printf("TMP xc_shutdown()");
    /* 执行清理操作 */
    xc.gc();
}