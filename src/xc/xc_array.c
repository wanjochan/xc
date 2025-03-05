/*
 * xc_array.c - 数组类型实现
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 从xc_string.c导入的函数 */
static const char* get_string_data(xc_val obj) {
    if (!obj || !xc.is(obj, XC_TYPE_STRING)) return "";
    
    /* 获取字符串数据 - 这里我们假设字符串对象的结构 */
    typedef struct {
        char* data;
        size_t length;
    } xc_string_t;
    
    xc_string_t* str = (xc_string_t*)obj;
    return str->data ? str->data : "";
}

/* 数组类型数据结构定义 - 仅在本文件中可见 */
typedef struct {
    xc_val* items;
    int count;
    int capacity;
} xc_array_t;

/* 数组类型方法 */
static xc_val array_to_string(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 构建字符串 "[elem1, elem2, ...]" */
    xc_val result = xc.create(XC_TYPE_STRING, "[");
    
    for (int i = 0; i < arr->count; i++) {
        /* 获取元素的字符串表示 */
        xc_val item_str;
        if (arr->items[i] == NULL) {
            item_str = xc.create(XC_TYPE_STRING, "null");
        } else {
            item_str = xc.call(arr->items[i], "toString");
            if (!item_str) {
                item_str = xc.create(XC_TYPE_STRING, "<unknow>");
            }
        }
        
        /* 拼接到结果中 */
        xc_val temp = xc.call(result, "concat", item_str);
        xc.release(result);
        result = temp;
        xc.release(item_str);
        
        /* 添加分隔符 */
        if (i < arr->count - 1) {
            xc_val comma = xc.create(XC_TYPE_STRING, ", ");
            temp = xc.call(result, "concat", comma);
            xc.release(result);
            result = temp;
            xc.release(comma);
        }
    }
    
    /* 添加结束括号 */
    xc_val bracket = xc.create(XC_TYPE_STRING, "]");
    xc_val temp = xc.call(result, "concat", bracket);
    xc.release(result);
    result = temp;
    xc.release(bracket);
    
    return result;
}

/* 获取数组长度 */
static xc_val array_length(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    return xc.create(XC_TYPE_NUMBER, (double)arr->count);
}

/* 获取数组元素 */
static xc_val array_get(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 检查索引类型 */
    if (!xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL; /* 索引必须是数字 */
    }
    
    /* 获取索引值并检查范围 */
    int index = (int)*(double*)arg;
    if (index < 0 || index >= arr->count) {
        return NULL; /* 索引越界 */
    }
    
    /* 返回元素（增加引用计数） */
    return xc.retain(arr->items[index]);
}

/* 设置数组元素 */
static xc_val array_set(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 检查参数 */
    if (!arg || !xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL; /* 第一个参数必须是索引 */
    }
    
    /* 获取索引值 */
    int index = (int)*(double*)arg;
    
    /* 获取值参数 */
    xc_val value = va_arg(*(va_list*)arg, xc_val);
    
    /* 检查索引范围并扩展数组 */
    if (index < 0) {
        return NULL; /* 索引不能为负 */
    }
    
    /* 如果索引超出当前容量，扩展数组 */
    if (index >= arr->capacity) {
        int new_capacity = index + 8; /* 预留一些空间 */
        xc_val* new_items = (xc_val*)realloc(arr->items, new_capacity * sizeof(xc_val));
        if (!new_items) {
            return NULL; /* 内存分配失败 */
        }
        
        /* 初始化新分配的元素为NULL */
        for (int i = arr->capacity; i < new_capacity; i++) {
            new_items[i] = NULL;
        }
        
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    
    /* 更新元素 */
    if (arr->items[index]) {
        xc.release(arr->items[index]);
    }
    
    arr->items[index] = xc.retain(value);
    
    /* 更新数组长度 */
    if (index >= arr->count) {
        arr->count = index + 1;
    }
    
    return value;
}

/* 添加元素到数组末尾 */
static xc_val array_push(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 获取值参数 */
    xc_val value = arg;
    
    /* 检查是否需要扩容 */
    if (arr->count >= arr->capacity) {
        int new_capacity = arr->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        
        xc_val* new_items = (xc_val*)realloc(arr->items, new_capacity * sizeof(xc_val));
        if (!new_items) {
            return NULL; /* 内存分配失败 */
        }
        
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    
    /* 添加元素 */
    arr->items[arr->count++] = xc.retain(value);
    
    /* 返回数组本身 */
    return self;
}

/* 从数组末尾移除元素 */
static xc_val array_pop(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 检查数组是否为空 */
    if (arr->count <= 0) {
        return NULL;
    }
    
    /* 获取最后一个元素 */
    xc_val last = arr->items[--arr->count];
    arr->items[arr->count] = NULL; /* 清空引用 */
    
    /* 返回元素（不增加引用计数，因为我们转移了所有权） */
    return last;
}

/* 数组类型标记函数 - 用于垃圾回收 */
static void array_mark(xc_val obj, void (*mark_func)(xc_val)) {
    xc_array_t* arr = (xc_array_t*)obj;
    
    /* 标记所有元素 */
    for (int i = 0; i < arr->count; i++) {
        if (arr->items[i]) {
            mark_func(arr->items[i]);
        }
    }
}

/* 数组类型终结器 - 用于释放资源 */
static int array_destroy(xc_val obj) {
    xc_array_t* arr = (xc_array_t*)obj;
    
    /* 释放所有元素 */
    for (int i = 0; i < arr->count; i++) {
        if (arr->items[i]) {
            xc.release(arr->items[i]);
            arr->items[i] = NULL;
        }
    }
    
    /* 释放数组内存 */
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
    
    return 1; /* 成功 */
}

/* indexOf方法 - 查找元素位置 */
static xc_val array_indexOf(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    xc_val search_item = arg;
    
    for (int i = 0; i < arr->count; i++) {
        /* 简单判断是否相等（直接使用指针比较，非常基础） */
        /* 未来应当改进为使用xc_equal函数进行比较 */
        if (arr->items[i] == search_item) {
            return xc.create(XC_TYPE_NUMBER, (double)i);
        }
    }
    
    /* 未找到返回-1 */
    return xc.create(XC_TYPE_NUMBER, -1.0);
}

/* slice方法 - 返回数组的一个片段 */
static xc_val array_slice(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 获取起始索引 */
    if (!arg || !xc.is(arg, XC_TYPE_NUMBER)) {
        return NULL; /* 起始索引必须是数字 */
    }
    int start = (int)*(double*)arg;
    
    /* 获取结束索引（可选） */
    int end = arr->count;
    xc_val end_val = va_arg(*(va_list*)arg, xc_val);
    if (end_val && xc.is(end_val, XC_TYPE_NUMBER)) {
        end = (int)*(double*)end_val;
    }
    
    /* 处理负索引 */
    if (start < 0) start = arr->count + start;
    if (end < 0) end = arr->count + end;
    
    /* 确保索引在有效范围内 */
    if (start < 0) start = 0;
    if (end > arr->count) end = arr->count;
    if (start >= end) return xc.create(XC_TYPE_ARRAY, 0);
    
    /* 创建新数组 */
    xc_val result = xc.create(XC_TYPE_ARRAY, end - start);
    
    /* 复制元素 */
    for (int i = start; i < end; i++) {
        xc.call(result, "push", arr->items[i]);
    }
    
    return result;
}

/* 数组连接为字符串 */
static xc_val array_join(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 获取分隔符参数 */
    const char* separator = ","; /* 默认分隔符 */
    xc_val separator_val = arg;
    if (separator_val && xc.is(separator_val, XC_TYPE_STRING)) {
        separator = get_string_data(separator_val);
    }
    
    /* 构建结果字符串 */
    xc_val result = xc.create(XC_TYPE_STRING, "");
    
    /* 连接元素 */
    for (int i = 0; i < arr->count; i++) {
        /* 获取元素的字符串表示 */
        xc_val item_str;
        if (arr->items[i] == NULL) {
            item_str = xc.create(XC_TYPE_STRING, "null");
        } else {
            item_str = xc.call(arr->items[i], "toString");
            if (!item_str) {
                item_str = xc.create(XC_TYPE_STRING, "<unknown>");
            }
        }
        
        /* 拼接到结果中 */
        xc_val temp = xc.call(result, "concat", item_str);
        xc.release(result);
        result = temp;
        xc.release(item_str);
        
        /* 添加分隔符（除了最后一个元素） */
        if (i < arr->count - 1) {
            xc_val sep_str = xc.create(XC_TYPE_STRING, separator);
            temp = xc.call(result, "concat", sep_str);
            xc.release(result);
            result = temp;
            xc.release(sep_str);
        }
    }
    
    return result;
}

/* forEach方法 - 对数组中的每个元素执行回调函数 */
static xc_val array_forEach(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 获取回调函数 */
    xc_val callback = arg;
    if (!callback || !xc.is(callback, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    /* 获取可选的闭包参数 */
    xc_val closure = va_arg(*(va_list*)arg, xc_val);
    
    /* 遍历数组元素 */
    for (int i = 0; i < arr->count; i++) {
        /* 准备参数：当前元素、索引、数组本身 */
        xc_val index = xc.create(XC_TYPE_NUMBER, (double)i);
        
        /* 调用回调函数 */
        xc_val result = xc.call(callback, "call", NULL, arr->items[i], index, self);
        
        /* 释放临时对象 */
        xc.release(index);
        if (result) xc.release(result);
    }
    
    /* 返回数组本身 */
    return self;
}

/* map方法 - 对数组中的每个元素执行回调函数并返回新数组 */
static xc_val array_map(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 获取回调函数 */
    xc_val callback = arg;
    if (!callback || !xc.is(callback, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    /* 获取可选的闭包参数 */
    xc_val closure = va_arg(*(va_list*)arg, xc_val);
    
    /* 创建结果数组 */
    xc_val result = xc.create(XC_TYPE_ARRAY, 0);
    
    /* 遍历数组元素 */
    for (int i = 0; i < arr->count; i++) {
        /* 准备参数：当前元素、索引、数组本身 */
        xc_val index = xc.create(XC_TYPE_NUMBER, (double)i);
        
        /* 调用回调函数 */
        xc_val mapped = xc.call(callback, "call", NULL, arr->items[i], index, self);
        
        /* 添加到结果数组 */
        xc.call(result, "push", mapped ? mapped : NULL);
        
        /* 释放临时对象 */
        xc.release(index);
        if (mapped) xc.release(mapped);
    }
    
    return result;
}

/* filter方法 - 过滤数组元素 */
static xc_val array_filter(xc_val self, xc_val arg) {
    xc_array_t* arr = (xc_array_t*)self;
    
    /* 获取回调函数 */
    xc_val callback = arg;
    if (!callback || !xc.is(callback, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    /* 获取可选的闭包参数 */
    xc_val closure = va_arg(*(va_list*)arg, xc_val);
    
    /* 创建结果数组 */
    xc_val result = xc.create(XC_TYPE_ARRAY, 0);
    
    /* 遍历数组元素 */
    for (int i = 0; i < arr->count; i++) {
        /* 准备参数：当前元素、索引、数组本身 */
        xc_val index = xc.create(XC_TYPE_NUMBER, (double)i);
        
        /* 调用回调函数 */
        xc_val test_result = xc.call(callback, "call", NULL, arr->items[i], index, self);
        
        /* 如果回调返回true，添加元素到结果数组 */
        if (test_result && xc.is(test_result, XC_TYPE_BOOL) && *(bool*)test_result) {
            xc.call(result, "push", arr->items[i]);
        }
        
        /* 释放临时对象 */
        xc.release(index);
        if (test_result) xc.release(test_result);
    }
    
    return result;
}

/* 数组类型初始化 */
static void array_initialize(void) {
    /* 注册数组方法 */
    xc.register_method(XC_TYPE_ARRAY, "toString", array_to_string);
    xc.register_method(XC_TYPE_ARRAY, "length", array_length);
    xc.register_method(XC_TYPE_ARRAY, "get", array_get);
    xc.register_method(XC_TYPE_ARRAY, "set", array_set);
    xc.register_method(XC_TYPE_ARRAY, "push", array_push);
    xc.register_method(XC_TYPE_ARRAY, "pop", array_pop);
    
    /* 高级方法 */
    xc.register_method(XC_TYPE_ARRAY, "indexOf", array_indexOf);
    xc.register_method(XC_TYPE_ARRAY, "slice", array_slice);
    xc.register_method(XC_TYPE_ARRAY, "join", array_join);
    xc.register_method(XC_TYPE_ARRAY, "forEach", array_forEach);
    xc.register_method(XC_TYPE_ARRAY, "map", array_map);
    xc.register_method(XC_TYPE_ARRAY, "filter", array_filter);
}

/* 数组类型创建函数 */
static xc_val array_creator(int type, va_list args) {
    /* 分配数组对象 */
    xc_array_t* arr = (xc_array_t*)malloc(sizeof(xc_array_t));
    if (!arr) return NULL;
    
    /* 初始化数组 */
    int initial_capacity = va_arg(args, int);
    if (initial_capacity < 0) initial_capacity = 0;
    
    /* 分配元素数组 */
    arr->capacity = initial_capacity > 0 ? initial_capacity : 8;
    arr->count = 0;
    arr->items = (xc_val*)malloc(arr->capacity * sizeof(xc_val));
    
    if (!arr->items) {
        free(arr);
        return NULL;
    }
    
    /* 初始化元素为NULL */
    for (int i = 0; i < arr->capacity; i++) {
        arr->items[i] = NULL;
    }
    
    return (xc_val)arr;
}

/* 数组类型生命周期 */
static xc_type_lifecycle_t array_lifecycle = {
    .initializer = array_initialize,
    .cleaner = NULL,
    .creator = array_creator,
    .destroyer = array_destroy,
    .allocator = NULL,
    .marker = array_mark
};

/* 注册数组类型 */
void xc_array_register(void) {
    xc.register_type("array", &array_lifecycle);
}

/* 获取数组长度 */
int xc_array_get_length(xc_val obj) {
    if (!obj || !xc.is(obj, XC_TYPE_ARRAY)) {
        return -1;
    }
    
    xc_array_t* arr = (xc_array_t*)obj;
    return arr->count;
}

/* 获取数组元素 */
xc_val xc_array_get_item(xc_val obj, int index) {
    if (!obj || !xc.is(obj, XC_TYPE_ARRAY)) {
        return NULL;
    }
    
    xc_array_t* arr = (xc_array_t*)obj;
    
    if (index < 0 || index >= arr->count) {
        return NULL; /* 索引越界 */
    }
    
    return xc.retain(arr->items[index]);
}

/* 设置数组元素 */
bool xc_array_set_item(xc_val obj, int index, xc_val value) {
    if (!obj || !xc.is(obj, XC_TYPE_ARRAY)) {
        return false;
    }
    
    xc_array_t* arr = (xc_array_t*)obj;
    
    if (index < 0) {
        return false; /* 索引不能为负 */
    }
    
    /* 如果索引超出当前容量，扩展数组 */
    if (index >= arr->capacity) {
        int new_capacity = index + 8; /* 预留一些空间 */
        xc_val* new_items = (xc_val*)realloc(arr->items, new_capacity * sizeof(xc_val));
        if (!new_items) {
            return false; /* 内存分配失败 */
        }
        
        /* 初始化新分配的元素为NULL */
        for (int i = arr->capacity; i < new_capacity; i++) {
            new_items[i] = NULL;
        }
        
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    
    /* 更新元素 */
    if (arr->items[index]) {
        xc.release(arr->items[index]);
    }
    
    arr->items[index] = xc.retain(value);
    
    /* 更新数组长度 */
    if (index >= arr->count) {
        arr->count = index + 1;
    }
    
    return true;
}

/* 添加数组元素 */
bool xc_array_add_item(xc_val obj, xc_val value) {
    if (!obj || !xc.is(obj, XC_TYPE_ARRAY)) {
        return false;
    }
    
    xc_array_t* arr = (xc_array_t*)obj;
    
    /* 检查是否需要扩容 */
    if (arr->count >= arr->capacity) {
        int new_capacity = arr->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        
        xc_val* new_items = (xc_val*)realloc(arr->items, new_capacity * sizeof(xc_val));
        if (!new_items) {
            return false; /* 内存分配失败 */
        }
        
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    
    /* 添加元素 */
    arr->items[arr->count++] = xc.retain(value);
    
    return true;
}