/*
 * xc_std_console.h - 控制台标准库头文件
 */

#ifndef XC_STD_CONSOLE_H
#define XC_STD_CONSOLE_H

#include "../xc.h"

/* 获取Console对象 */
xc_val xc_std_get_console(void);

/* 初始化Console库 */
void xc_std_console_initialize(void);

/* 清理Console库 */
void xc_std_console_cleanup(void);

#endif /* XC_STD_CONSOLE_H */ 