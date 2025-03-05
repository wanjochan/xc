/*
 * xc_string.c - XC String Type Implementation
 */

#include "xc.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* 字符串数据结构 */
typedef struct {
    char* data;     /* 字符串数据 */
    size_t length;  /* 字符串长度 */
} xc_string_t;

/* 获取字符串数据的安全函数 */
static const char* get_string_data(xc_val obj) {
    if (!obj || !xc.is(obj, XC_TYPE_STRING)) return "";
    xc_string_t* str = (xc_string_t*)obj;
    return str->data ? str->data : "";
}

/* String methods */
static xc_val string_length(xc_val self, xc_val arg) {
    xc_string_t* str = (xc_string_t*)self;
    return xc.create(XC_TYPE_NUMBER, (double)str->length);
}

static xc_val string_indexOf(xc_val self, xc_val arg) {
    if (!arg || !xc.is(arg, XC_TYPE_STRING)) {
        return xc.create(XC_TYPE_NUMBER, -1.0);
    }
    
    const char* str = get_string_data(self);
    const char* sub = get_string_data(arg);
    const char* pos = strstr(str, sub);
    
    if (pos) {
        return xc.create(XC_TYPE_NUMBER, (double)(pos - str));
    }
    return xc.create(XC_TYPE_NUMBER, -1.0);
}

static xc_val string_substring(xc_val self, xc_val arg) {
    if (!arg || !xc.is(arg, XC_TYPE_NUMBER)) {
        return self;
    }
    
    const char* str = get_string_data(self);
    int start = (int)(*(double*)arg);
    int len = strlen(str);
    
    if (start < 0) start = 0;
    if (start >= len) return xc.create(XC_TYPE_STRING, "");
    
    return xc.create(XC_TYPE_STRING, str + start);
}

static xc_val string_split(xc_val self, xc_val arg) {
    const char* str = get_string_data(self);
    const char* delim = arg && xc.is(arg, XC_TYPE_STRING) ? get_string_data(arg) : " ";
    
    xc_val array = xc.create(XC_TYPE_ARRAY);
    if (!array) return NULL;
    
    if (!*str) {
        xc_val empty = xc.create(XC_TYPE_STRING, "");
        if (empty) xc.call(array, "push", empty);
        return array;
    }
    
    char* temp = strdup(str);
    if (!temp) return array;
    
    char* token = strtok(temp, delim);
    while (token) {
        xc_val item = xc.create(XC_TYPE_STRING, token);
        if (item) xc.call(array, "push", item);
        token = strtok(NULL, delim);
    }
    
    free(temp);
    return array;
}

static xc_val string_trim(xc_val self, xc_val arg) {
    const char* str = get_string_data(self);
    int len = strlen(str);
    
    int start = 0;
    while (start < len && isspace(str[start])) start++;
    
    int end = len - 1;
    while (end >= start && isspace(str[end])) end--;
    
    if (start > end) return xc.create(XC_TYPE_STRING, "");
    
    int new_len = end - start + 1;
    char* new_str = malloc(new_len + 1);
    if (!new_str) return self;
    
    strncpy(new_str, str + start, new_len);
    new_str[new_len] = '\0';
    
    xc_val result = xc.create(XC_TYPE_STRING, new_str);
    free(new_str);
    
    return result;
}

static xc_val string_toLowerCase(xc_val self, xc_val arg) {
    const char* str = get_string_data(self);
    int len = strlen(str);
    
    char* new_str = malloc(len + 1);
    if (!new_str) return self;
    
    for (int i = 0; i < len; i++) {
        new_str[i] = tolower(str[i]);
    }
    new_str[len] = '\0';
    
    xc_val result = xc.create(XC_TYPE_STRING, new_str);
    free(new_str);
    
    return result;
}

static xc_val string_toUpperCase(xc_val self, xc_val arg) {
    const char* str = get_string_data(self);
    int len = strlen(str);
    
    char* new_str = malloc(len + 1);
    if (!new_str) return self;
    
    for (int i = 0; i < len; i++) {
        new_str[i] = toupper(str[i]);
    }
    new_str[len] = '\0';
    
    xc_val result = xc.create(XC_TYPE_STRING, new_str);
    free(new_str);
    
    return result;
}

/* String object creation */
static xc_val string_creator(int type, va_list args) {
    const char* str = va_arg(args, const char*);
    if (!str) str = "";
    
    xc_val obj = xc.alloc_object(type, sizeof(xc_string_t));
    if (!obj) return NULL;
    
    xc_string_t* string = (xc_string_t*)obj;
    size_t len = strlen(str);
    
    string->data = malloc(len + 1);
    if (!string->data) {
        free(obj);
        return NULL;
    }
    
    strcpy(string->data, str);
    string->length = len;
    
    return obj;
}

/* String object cleanup */
static int string_destroyer(xc_val obj) {
    if (obj) {
        xc_string_t* string = (xc_string_t*)obj;
        if (string->data) {
            free(string->data);
        }
        return 1;
    }
    return 0;
}

/* String mark function for GC */
static void string_marker(xc_val obj, void (*mark_func)(xc_val)) {
    /* 字符串对象不包含其他对象引用 */
}

/* String type registration */
static void string_initialize(void) {
    /* Register string methods */
    xc.register_method(XC_TYPE_STRING, "length", string_length);
    xc.register_method(XC_TYPE_STRING, "indexOf", string_indexOf);
    xc.register_method(XC_TYPE_STRING, "substring", string_substring);
    xc.register_method(XC_TYPE_STRING, "split", string_split);
    xc.register_method(XC_TYPE_STRING, "trim", string_trim);
    xc.register_method(XC_TYPE_STRING, "toLowerCase", string_toLowerCase);
    xc.register_method(XC_TYPE_STRING, "toUpperCase", string_toUpperCase);
}

/* String type lifecycle */
static xc_type_lifecycle_t string_lifecycle = {
    .initializer = string_initialize,
    .cleaner = NULL,
    .creator = string_creator,
    .destroyer = string_destroyer,
    .allocator = NULL,
    .marker = string_marker
};

/* Register string type - 通过xc_init调用 */
void xc_string_register(void) {
    xc.register_type("string", &string_lifecycle);
}