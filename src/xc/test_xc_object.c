/*
 * test_xc_object.c - Object类型方法测试
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 测试Object类型方法 */
void test_object_methods(void) {
    printf("\n=== 测试Object类型方法 ===\n");
    
    /* 创建测试对象 */
    xc_val obj = xc.create(XC_TYPE_OBJECT, 0);
    
    /* 测试set和get */
    xc_val key1 = xc.create(XC_TYPE_STRING, "name");
    xc_val value1 = xc.create(XC_TYPE_STRING, "XC Runtime");
    xc.call(obj, "set", key1, value1);
    xc.release(key1);
    xc.release(value1);
    
    xc_val key2 = xc.create(XC_TYPE_STRING, "version");
    xc_val value2 = xc.create(XC_TYPE_NUMBER, 1.0);
    xc.call(obj, "set", key2, value2);
    xc.release(key2);
    xc.release(value2);
    
    xc_val key3 = xc.create(XC_TYPE_STRING, "active");
    xc_val value3 = xc.create(XC_TYPE_BOOLEAN, 1);
    xc.call(obj, "set", key3, value3);
    xc.release(key3);
    xc.release(value3);
    
    /* 测试get */
    key1 = xc.create(XC_TYPE_STRING, "name");
    xc_val result = xc.call(obj, "get", key1);
    printf("obj.name = %s\n", (char*)result);
    assert(strcmp((char*)result, "XC Runtime") == 0);
    xc.release(key1);
    
    /* 测试has */
    key1 = xc.create(XC_TYPE_STRING, "name");
    xc_val has_result = xc.call(obj, "has", key1);
    printf("obj.has('name') = %s\n", *(int*)has_result ? "true" : "false");
    assert(*(int*)has_result == 1);
    xc.release(has_result);
    
    key2 = xc.create(XC_TYPE_STRING, "unknown");
    has_result = xc.call(obj, "has", key2);
    printf("obj.has('unknown') = %s\n", *(int*)has_result ? "true" : "false");
    assert(*(int*)has_result == 0);
    xc.release(has_result);
    xc.release(key1);
    xc.release(key2);
    
    /* 测试toString */
    xc_val str = xc.call(obj, "toString");
    printf("obj.toString() = %s\n", (char*)str);
    xc.release(str);
    
    /* 测试keys */
    xc_val keys = xc.call(obj, "keys");
    xc_val keys_str = xc.call(keys, "toString");
    printf("obj.keys() = %s\n", (char*)keys_str);
    
    xc_val length = xc.call(keys, "length");
    assert(*(double*)length == 3.0);
    xc.release(length);
    xc.release(keys_str);
    xc.release(keys);
    
    /* 测试values */
    xc_val values = xc.call(obj, "values");
    xc_val values_str = xc.call(values, "toString");
    printf("obj.values() = %s\n", (char*)values_str);
    
    length = xc.call(values, "length");
    assert(*(double*)length == 3.0);
    xc.release(length);
    xc.release(values_str);
    xc.release(values);
    
    /* 测试entries */
    xc_val entries = xc.call(obj, "entries");
    xc_val entries_str = xc.call(entries, "toString");
    printf("obj.entries() = %s\n", (char*)entries_str);
    
    length = xc.call(entries, "length");
    assert(*(double*)length == 3.0);
    xc.release(length);
    
    /* 验证entries的结构 */
    xc_val index0 = xc.create(XC_TYPE_NUMBER, 0.0);
    xc_val first_entry = xc.call(entries, "get", index0);
    xc_val first_entry_str = xc.call(first_entry, "toString");
    printf("First entry: %s\n", (char*)first_entry_str);
    xc.release(first_entry_str);
    
    length = xc.call(first_entry, "length");
    assert(*(double*)length == 2.0); /* 每个entry应该是[key, value]格式 */
    xc.release(length);
    xc.release(first_entry);
    xc.release(index0);
    xc.release(entries_str);
    xc.release(entries);
    
    /* 测试delete */
    key1 = xc.create(XC_TYPE_STRING, "version");
    xc_val delete_result = xc.call(obj, "delete", key1);
    printf("obj.delete('version') = %s\n", *(int*)delete_result ? "true" : "false");
    assert(*(int*)delete_result == 1);
    xc.release(delete_result);
    
    /* 验证删除后的状态 */
    has_result = xc.call(obj, "has", key1);
    printf("After delete, obj.has('version') = %s\n", *(int*)has_result ? "true" : "false");
    assert(*(int*)has_result == 0);
    xc.release(has_result);
    xc.release(key1);
    
    /* 再次获取keys，验证数量 */
    keys = xc.call(obj, "keys");
    length = xc.call(keys, "length");
    printf("After delete, keys.length = %d\n", (int)*(double*)length);
    assert(*(double*)length == 2.0); /* 应该只剩下2个键 */
    xc.release(length);
    xc.release(keys);
    
    /* 清理 */
    xc.release(obj);
    
    printf("Object类型方法测试通过!\n");
}

/* 主函数 */
int main(int argc, char** argv) {
    /* 初始化XC运行时 */
    xc_initialize();
    
    /* 运行测试 */
    test_object_methods();
    
    /* 清理XC运行时 */
    xc_cleanup();
    
    return 0;
} 