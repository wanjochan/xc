/*
 * xc_std_console.c - 控制台标准库实现
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* Console对象 */
static xc_val console_obj = NULL;

/* 用于格式化日志消息的辅助函数 */
static char* format_message(int argc, xc_val* argv) {
    if (argc == 0) {
        return strdup("");
    }
    
    /* 为消息分配初始缓冲区 */
    size_t buffer_size = 1024;
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) {
        return NULL;
    }
    
    size_t current_len = 0;
    buffer[0] = '\0';
    
    /* 连接所有参数的字符串表示 */
    for (int i = 0; i < argc; i++) {
        xc_val item_str;
        if (argv[i]) {
            item_str = xc.call(argv[i], "toString");
            if (!item_str) {
                item_str = xc.create(XC_TYPE_STRING, "<unknown>");
            }
        } else {
            item_str = xc.create(XC_TYPE_STRING, "null");
        }
        
        const char* str = xc_string_get_buffer(item_str);
        size_t len = strlen(str);
        
        /* 检查是否需要扩展缓冲区 */
        if (current_len + len + 2 > buffer_size) {
            buffer_size = (current_len + len + 1024) * 2;
            char* new_buffer = (char*)realloc(buffer, buffer_size);
            if (!new_buffer) {
                free(buffer);
                xc.release(item_str);
                return NULL;
            }
            buffer = new_buffer;
        }
        
        /* 添加空格分隔符（除了第一个元素） */
        if (i > 0) {
            strcat(buffer, " ");
            current_len++;
        }
        
        /* 添加字符串 */
        strcat(buffer, str);
        current_len += len;
        
        xc.release(item_str);
    }
    
    return buffer;
}

/* console.log - 日志输出 */
static xc_val console_log(xc_val self, int argc, xc_val* argv, xc_val closure) {
    char* message = format_message(argc, argv);
    if (message) {
        printf("%s\n", message);
        free(message);
    }
    
    return NULL;
}

/* console.error - 错误输出 */
static xc_val console_error(xc_val self, int argc, xc_val* argv, xc_val closure) {
    char* message = format_message(argc, argv);
    if (message) {
        fprintf(stderr, "[ERROR] %s\n", message);
        free(message);
    }
    
    return NULL;
}

/* console.warn - 警告输出 */
static xc_val console_warn(xc_val self, int argc, xc_val* argv, xc_val closure) {
    char* message = format_message(argc, argv);
    if (message) {
        fprintf(stderr, "[WARNING] %s\n", message);
        free(message);
    }
    
    return NULL;
}

/* console.time - 开始计时 */
static xc_val console_time(xc_val self, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !xc.is(argv[0], XC_TYPE_STRING)) {
        return NULL;
    }
    
    const char* label = xc_string_get_buffer(argv[0]);
    
    /* 存储当前时间到label属性中 */
    clock_t start_time = clock();
    xc_val time_value = xc.create(XC_TYPE_NUMBER, (double)start_time);
    xc_object_set_property(self, label, time_value);
    xc.release(time_value);
    
    return NULL;
}

/* console.timeEnd - 结束计时并打印耗时 */
static xc_val console_timeEnd(xc_val self, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !xc.is(argv[0], XC_TYPE_STRING)) {
        return NULL;
    }
    
    const char* label = xc_string_get_buffer(argv[0]);
    
    /* 获取存储的开始时间 */
    xc_val start_val = xc_object_get_property(self, label);
    if (!start_val || !xc.is(start_val, XC_TYPE_NUMBER)) {
        printf("%s: <timer not started>\n", label);
        return NULL;
    }
    
    /* 计算并打印耗时 */
    clock_t start_time = (clock_t)*(double*)start_val;
    clock_t end_time = clock();
    double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0; /* 毫秒 */
    
    printf("%s: %.2f ms\n", label, elapsed);
    
    /* 移除存储的时间 */
    xc_object_set_property(self, label, NULL);
    
    return NULL;
}

/* 创建Console对象 */
static xc_val create_console_object(void) {
    xc_val obj = xc.create(XC_TYPE_OBJECT);
    
    /* 添加方法 */
    xc_val log_func = xc.create(XC_TYPE_FUNC, console_log, -1, NULL);
    xc_object_set_property(obj, "log", log_func);
    xc.release(log_func);
    
    xc_val error_func = xc.create(XC_TYPE_FUNC, console_error, -1, NULL);
    xc_object_set_property(obj, "error", error_func);
    xc.release(error_func);
    
    xc_val warn_func = xc.create(XC_TYPE_FUNC, console_warn, -1, NULL);
    xc_object_set_property(obj, "warn", warn_func);
    xc.release(warn_func);
    
    xc_val time_func = xc.create(XC_TYPE_FUNC, console_time, 1, NULL);
    xc_object_set_property(obj, "time", time_func);
    xc.release(time_func);
    
    xc_val timeEnd_func = xc.create(XC_TYPE_FUNC, console_timeEnd, 1, NULL);
    xc_object_set_property(obj, "timeEnd", timeEnd_func);
    xc.release(timeEnd_func);
    
    return obj;
}

/* 获取Console对象 */
xc_val xc_std_get_console(void) {
    if (console_obj == NULL) {
        console_obj = create_console_object();
    }
    return console_obj;
}

/* 初始化Console库 */
void xc_std_console_initialize(void) {
    /* 创建Console对象 */
    console_obj = create_console_object();
    
    /* 将Console对象添加到全局对象 */
    xc_val global = xc.get_global_object();
    xc_object_set_property(global, "console", console_obj);
}

/* 清理Console库 */
void xc_std_console_cleanup(void) {
    if (console_obj != NULL) {
        xc.release(console_obj);
        console_obj = NULL;
    }
}

/* 使用构造函数自动初始化Console库 */
__attribute__((constructor)) static void register_console_lib(void) {
    printf("register_console_lib()\n");
} 