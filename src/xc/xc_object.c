/*
 * xc_object.c - 对象类型实现
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

/* 对象属性条目 */
typedef struct {
    char* key;
    xc_val value;
} xc_object_entry_t;

/* 对象类型数据结构定义 - 仅在本文件中可见 */
typedef struct {
    xc_object_entry_t* entries;
    int count;
    int capacity;
} xc_object_t;

/* 对象类型方法 */
static xc_val object_to_string(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    /* 构建字符串 "{key1: value1, key2: value2, ...}" */
    xc_val result = xc.create(XC_TYPE_STRING, "{");
    
    for (int i = 0; i < obj->count; i++) {
        /* 添加键 */
        xc_val key_str = xc.create(XC_TYPE_STRING, obj->entries[i].key);
        xc_val temp = xc.call(result, "concat", key_str);
        xc.release(result);
        result = temp;
        xc.release(key_str);
        
        /* 添加分隔符 */
        xc_val colon = xc.create(XC_TYPE_STRING, ": ");
        temp = xc.call(result, "concat", colon);
        xc.release(result);
        result = temp;
        xc.release(colon);
        
        /* 获取值的字符串表示 */
        xc_val val_str;
        if (obj->entries[i].value == NULL) {
            val_str = xc.create(XC_TYPE_STRING, "null");
        } else {
            val_str = xc.call(obj->entries[i].value, "toString");
            if (!val_str) {
                val_str = xc.create(XC_TYPE_STRING, "<unknown>");
            }
        }
        
        /* 添加值 */
        temp = xc.call(result, "concat", val_str);
        xc.release(result);
        result = temp;
        xc.release(val_str);
        
        /* 添加分隔符 */
        if (i < obj->count - 1) {
            xc_val comma = xc.create(XC_TYPE_STRING, ", ");
            temp = xc.call(result, "concat", comma);
            xc.release(result);
            result = temp;
            xc.release(comma);
        }
    }
    
    /* 添加结束大括号 */
    xc_val brace = xc.create(XC_TYPE_STRING, "}");
    xc_val temp = xc.call(result, "concat", brace);
    xc.release(result);
    xc.release(brace);
    
    return temp;
}

static xc_val object_keys(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    /* 创建键数组 */
    xc_val keys = xc.create(XC_TYPE_ARRAY, obj->count);
    
    for (int i = 0; i < obj->count; i++) {
        xc_val key = xc.create(XC_TYPE_STRING, obj->entries[i].key);
        xc.call(keys, "push", key);
        xc.release(key);
    }
    
    return keys;
}

static xc_val object_values(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    /* 创建值数组 */
    xc_val values = xc.create(XC_TYPE_ARRAY, obj->count);
    
    for (int i = 0; i < obj->count; i++) {
        xc.call(values, "push", obj->entries[i].value);
    }
    
    return values;
}

static xc_val object_entries(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    /* 创建条目数组 */
    xc_val entries = xc.create(XC_TYPE_ARRAY, obj->count);
    
    for (int i = 0; i < obj->count; i++) {
        /* 为每个键值对创建一个包含两个元素的数组 */
        xc_val pair = xc.create(XC_TYPE_ARRAY, 2);
        
        /* 添加键 */
        xc_val key = xc.create(XC_TYPE_STRING, obj->entries[i].key);
        xc.call(pair, "push", key);
        xc.release(key);
        
        /* 添加值 */
        xc.call(pair, "push", obj->entries[i].value);
        
        /* 将键值对添加到结果数组 */
        xc.call(entries, "push", pair);
        xc.release(pair);
    }
    
    return entries;
}

static xc_val object_delete(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    if (!xc.is(arg, XC_TYPE_STRING)) {
        return xc.create(XC_TYPE_BOOL, XC_FALSE);
    }
    
    const char* key = get_string_data(arg);
    
    /* 查找属性 */
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->entries[i].key, key) == 0) {
            /* 释放资源 */
            free(obj->entries[i].key);
            if (obj->entries[i].value) {
                xc.release(obj->entries[i].value);
            }
            
            /* 移动后面的元素 */
            if (i < obj->count - 1) {
                memmove(&obj->entries[i], &obj->entries[i + 1], 
                        (obj->count - i - 1) * sizeof(xc_object_entry_t));
            }
            
            /* 更新计数 */
            obj->count--;
            
            return xc.create(XC_TYPE_BOOL, XC_TRUE);
        }
    }
    
    return xc.create(XC_TYPE_BOOL, XC_FALSE);
}

static xc_val object_get(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    if (!xc.is(arg, XC_TYPE_STRING)) {
        return NULL;
    }
    
    const char* key = get_string_data(arg);
    
    /* 查找属性 */
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->entries[i].key, key) == 0) {
            return obj->entries[i].value;
        }
    }
    
    return NULL;
}

static xc_val object_set(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    if (!xc.is(arg, XC_TYPE_STRING)) {
        return NULL;
    }
    
    const char* key = get_string_data(arg);
    xc_val value = va_arg(*(va_list*)arg, xc_val);
    
    /* 查找是否已存在 */
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->entries[i].key, key) == 0) {
            /* 替换值 */
            xc_val old = obj->entries[i].value;
            obj->entries[i].value = value;
            
            /* 更新引用计数 */
            if (value) xc.retain(value);
            if (old) xc.release(old);
            
            return value;
        }
    }
    
    /* 检查是否需要扩容 */
    if (obj->count >= obj->capacity) {
        int new_cap = obj->capacity * 2;
        if (new_cap == 0) new_cap = 4;
        
        xc_object_entry_t* new_entries = (xc_object_entry_t*)realloc(
            obj->entries, new_cap * sizeof(xc_object_entry_t));
        if (!new_entries) {
            return NULL;
        }
        
        obj->entries = new_entries;
        obj->capacity = new_cap;
    }
    
    /* 添加新键值对 */
    obj->entries[obj->count].key = strdup(key);
    if (!obj->entries[obj->count].key) {
        return NULL;
    }
    
    obj->entries[obj->count].value = value;
    if (value) xc.retain(value);
    
    obj->count++;
    return value;
}

static xc_val object_has(xc_val self, xc_val arg) {
    xc_object_t* obj = (xc_object_t*)self;
    
    if (!xc.is(arg, XC_TYPE_STRING)) {
        return xc.create(XC_TYPE_BOOL, XC_FALSE);
    }
    
    const char* key = get_string_data(arg);
    
    /* 查找属性 */
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->entries[i].key, key) == 0) {
            return xc.create(XC_TYPE_BOOL, XC_TRUE);
        }
    }
    
    return xc.create(XC_TYPE_BOOL, XC_FALSE);
}

/* 对象类型标记函数 - 用于垃圾回收 */
static void object_mark(xc_val obj, void (*mark_func)(xc_val)) {
    xc_object_t* o = (xc_object_t*)obj;
    
    /* 标记所有值 */
    for (int i = 0; i < o->count; i++) {
        if (o->entries[i].value) {
            mark_func(o->entries[i].value);
        }
    }
}

/* 对象类型初始化 */
static void object_initialize(void) {
    /* 注册对象方法 */
    xc.register_method(XC_TYPE_OBJECT, "toString", object_to_string);
    xc.register_method(XC_TYPE_OBJECT, "keys", object_keys);
    xc.register_method(XC_TYPE_OBJECT, "values", object_values);
    xc.register_method(XC_TYPE_OBJECT, "entries", object_entries);
    xc.register_method(XC_TYPE_OBJECT, "get", object_get);
    xc.register_method(XC_TYPE_OBJECT, "set", object_set);
    xc.register_method(XC_TYPE_OBJECT, "has", object_has);
    xc.register_method(XC_TYPE_OBJECT, "delete", object_delete);
}

/* 对象类型创建函数 */
static xc_val object_creator(int type, va_list args) {
    /* 分配对象 */
    xc_object_t* obj = (xc_object_t*)malloc(sizeof(xc_object_t));
    if (!obj) return NULL;
    
    /* 初始化对象 */
    int initial_capacity = va_arg(args, int);
    if (initial_capacity < 0) initial_capacity = 0;
    
    /* 分配条目数组 */
    obj->capacity = initial_capacity > 0 ? initial_capacity : 4;
    obj->count = 0;
    obj->entries = (xc_object_entry_t*)malloc(obj->capacity * sizeof(xc_object_entry_t));
    
    if (!obj->entries) {
        free(obj);
        return NULL;
    }
    
    return (xc_val)obj;
}

/* 对象类型销毁函数 */
static int object_destroy(xc_val obj) {
    xc_object_t* o = (xc_object_t*)obj;
    
    /* 释放所有条目 */
    for (int i = 0; i < o->count; i++) {
        free(o->entries[i].key);
        if (o->entries[i].value) {
            xc.release(o->entries[i].value);
        }
    }
    
    /* 释放条目数组 */
    free(o->entries);
    o->entries = NULL;
    o->count = 0;
    o->capacity = 0;
    
    return 1; /* 成功 */
}

/* 对象类型生命周期 */
static xc_type_lifecycle_t object_lifecycle = {
    .initializer = object_initialize,
    .cleaner = NULL,
    .creator = object_creator,
    .destroyer = object_destroy,
    .allocator = NULL,
    .marker = object_mark
};

/* 注册对象类型 */
void xc_object_register(void) {
    xc.register_type("object", &object_lifecycle);
}