/*
 * xc_std_math.h - 数学标准库头文件
 */

#ifndef XC_STD_MATH_H
#define XC_STD_MATH_H

#include "../xc.h"

/* 获取Math对象 */
xc_val xc_std_get_math(void);

/* 初始化Math库 */
void xc_std_math_initialize(void);

/* 清理Math库 */
void xc_std_math_cleanup(void);

#endif /* XC_STD_MATH_H */ 