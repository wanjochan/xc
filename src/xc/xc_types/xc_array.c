/*
 * xc_array.c - Array type implementation
 */

#include "../xc.h"
#include "../xc_internal.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c

/* Forward declarations */
static bool array_ensure_capacity(xc_array_t *arr, size_t needed);
static xc_object_t *xc_to_string_internal(xc_runtime_t *rt, xc_object_t *obj);
static xc_object_t *xc_array_join_elements(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator);
int xc_array_find_index_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index);

/* Helper functions for type checking - these should match the xc_is_* functions in other files */
static bool xc_is_array_object(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_array(rt, obj);
}

static bool xc_is_string_object(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_string(rt, obj);
}

static bool xc_is_number_object(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_number(rt, obj);
}

static bool xc_is_boolean_object(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_boolean(rt, obj);
}

static bool xc_is_null_object(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_is_null(rt, obj);
}

/* Helper functions for value access - these should match the xc_*_value functions in other files */
static const char *xc_string_get_value(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_string_value(rt, obj);
}

static double xc_number_get_value(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_number_value(rt, obj);
}

static bool xc_boolean_get_value(xc_runtime_t *rt, xc_object_t *obj) {
    return xc_boolean_value(rt, obj);
}

/* Helper function for string creation */
static xc_object_t *xc_string_object_create(xc_runtime_t *rt, const char *str) {
    return xc_string_create(rt, str);
}

/* Helper function for object comparison */
static int xc_compare_objects(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    return xc_compare(rt, a, b);
}

/* Helper function for reference counting */
static xc_object_t *xc_gc_retain(xc_runtime_t *rt, xc_object_t *obj) {
    xc_gc_add_ref(rt, obj);
    return obj;
}

/* Array methods */
static void array_mark(xc_runtime_t *rt, xc_object_t *obj) {
    xc_array_t *arr = (xc_array_t *)obj;
    for (size_t i = 0; i < arr->length; i++) {
        if (arr->items[i]) {
            /* Mark each item in the array */
            xc_gc_mark(rt, arr->items[i]);
        }
    }
}

static void array_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_array_t *arr = (xc_array_t *)obj;
    /* Release all items */
    for (size_t i = 0; i < arr->length; i++) {
        if (arr->items[i]) {
            xc_gc_free(rt, arr->items[i]);
        }
    }
    /* Free the items array */
    free(arr->items);
    arr->items = NULL;
    arr->length = 0;
    arr->capacity = 0;
}

static bool array_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_array(rt, b)) {
        return false;
    }
    
    xc_array_t *arr_a = (xc_array_t *)a;
    xc_array_t *arr_b = (xc_array_t *)b;

    if (arr_a->length != arr_b->length) {
        return false;
    }

    for (size_t i = 0; i < arr_a->length; i++) {
        if (!xc_equal(rt, arr_a->items[i], arr_b->items[i])) {
            return false;
        }
    }
    
    return true;
}

static int array_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    if (!xc_is_array(rt, b)) {
        return 1; /* Arrays are greater than non-arrays */
    }
    
    xc_array_t *arr_a = (xc_array_t *)a;
    xc_array_t *arr_b = (xc_array_t *)b;

    size_t min_len = arr_a->length < arr_b->length ? arr_a->length : arr_b->length;

    for (size_t i = 0; i < min_len; i++) {
        int cmp = xc_compare(rt, arr_a->items[i], arr_b->items[i]);
        if (cmp != 0) {
            return cmp;
        }
    }

    if (arr_a->length < arr_b->length) return -1;
    if (arr_a->length > arr_b->length) return 1;
    return 0;
}

/* Array type structure */
static xc_type_t array_type = {
    .name = "array",
    .flags = XC_TYPE_COMPOSITE,
    .mark = array_mark,
    .free = array_free,
    .equal = array_equal,
    .compare = array_compare
};

/* Array creator function for type system */
static xc_val array_creator(int type, va_list args) {
    // 使用全局运行时实例
    xc_runtime_t *rt = &xc;
    printf("DEBUG array_creator called, type=%d\n", type);
    // 创建一个空数组
    return xc_array_create(rt);
}

/* 方法包装函数 */
static xc_val array_length_method(xc_val self, xc_val arg) {
    xc_runtime_t *rt = &xc;
    printf("DEBUG array_length_method called, self=%p\n", self);
    return (xc_val)xc_array_length(rt, (xc_object_t *)self);
}

static xc_val array_get_method(xc_val self, xc_val arg) {
    xc_runtime_t *rt = &xc;
    printf("DEBUG array_get_method called, self=%p, arg=%p\n", self, arg);
    // 参数应该是一个数字，表示索引
    if (!arg || !rt->is(arg, XC_TYPE_NUMBER)) {
        printf("DEBUG array_get_method: arg is not a number\n");
        return NULL;
    }
    // 获取索引值
    long index = (long)arg;
    printf("DEBUG array_get_method: index=%ld\n", index);
    return xc_array_get(rt, (xc_object_t *)self, index);
}

static xc_val array_push_method(xc_val self, xc_val arg) {
    xc_runtime_t *rt = &xc;
    printf("DEBUG array_push_method called, self=%p, arg=%p\n", self, arg);
    xc_array_push(rt, (xc_object_t *)self, (xc_object_t *)arg);
    return self; // 返回数组自身
}

/* Array initializer function for type system */
static void array_initializer() {
    xc_runtime_t *rt = &xc;
    // 数组类型的初始化逻辑
    printf("DEBUG array_initializer called\n");
    printf("DEBUG array_initializer: registering methods for type %d\n", XC_TYPE_ARRAY);
    printf("DEBUG array_initializer: length method at %p\n", array_length_method);
    printf("DEBUG array_initializer: get method at %p\n", array_get_method);
    printf("DEBUG array_initializer: push method at %p\n", array_push_method);
    /* 注册数组方法 */
    rt->register_method(XC_TYPE_ARRAY, "length", array_length_method);
    rt->register_method(XC_TYPE_ARRAY, "get", array_get_method);
    rt->register_method(XC_TYPE_ARRAY, "push", array_push_method);
    printf("DEBUG array_initializer: methods registered\n");
}

/* Register array type */
void xc_register_array_type(xc_runtime_t *rt) {
    printf("DEBUG xc_register_array_type: registering array type\n");
    
    /* 创建生命周期管理结构 */
    xc_type_lifecycle_t lifecycle = {
        .initializer = array_initializer,
        .cleaner = NULL,
        .creator = array_creator,
        .destroyer = NULL,
        .allocator = NULL,
        .marker = NULL
    };
    
    printf("DEBUG array creator=%p\n", array_creator);
    printf("DEBUG array initializer=%p\n", array_initializer);
    
    /* 注册类型 */
    int type_id = xc_register_type("array", &lifecycle);
    
    /* 使用 XC_RUNTIME_EXT 宏访问扩展运行时结构体 */
    XC_RUNTIME_EXT(rt)->array_type = &array_type;
    printf("DEBUG xc_register_array_type: set array_type to %p\n", &array_type);
    
    // /* 注册数组方法 */
    // rt->register_method(XC_TYPE_ARRAY, "length", array_length_method);
    // rt->register_method(XC_TYPE_ARRAY, "get", array_get_method);
    // rt->register_method(XC_TYPE_ARRAY, "push", array_push_method);
}

/* Ensure array has enough capacity */
static bool array_ensure_capacity(xc_array_t *arr, size_t needed) {
    if (needed <= arr->capacity) {
        return true;
    }

    size_t new_capacity = arr->capacity == 0 ? 8 : arr->capacity * 2;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    xc_object_t **new_items = realloc(arr->items, new_capacity * sizeof(xc_object_t *));
    if (!new_items) {
        return false;
    }

    /* Initialize new slots to NULL */
    for (size_t i = arr->capacity; i < new_capacity; i++) {
        new_items[i] = NULL;
    }

    arr->items = new_items;
    arr->capacity = new_capacity;
    return true;
}

/* Array creation */
xc_object_t *xc_array_create(xc_runtime_t *rt) {
    printf("DEBUG xc_array_create called\n");
    return xc_array_create_with_capacity(rt, 0);
}

/* Create array with initial values */
xc_object_t *xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count) {
    printf("DEBUG xc_array_create_with_values called, count=%zu\n", count);
    xc_object_t *arr = xc_array_create_with_capacity(rt, count);
    if (!arr) {
        printf("DEBUG xc_array_create_with_values: failed to create array\n");
        return NULL;
    }
    
    for (size_t i = 0; i < count; i++) {
        xc_array_push(rt, arr, values[i]);
    }
    
    return arr;
}

xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t initial_capacity) {
    printf("DEBUG xc_array_create_with_capacity called, initial_capacity=%zu\n", initial_capacity);
    printf("DEBUG xc_array_create_with_capacity: rt=%p\n", rt);
    printf("DEBUG xc_array_create_with_capacity: XC_RUNTIME_EXT(rt)=%p\n", XC_RUNTIME_EXT(rt));
    printf("DEBUG xc_array_create_with_capacity: array_type=%p\n", XC_RUNTIME_EXT(rt)->array_type);
    
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_array_t *arr = (xc_array_t *)xc_gc_alloc(rt, sizeof(xc_array_t), XC_TYPE_ARRAY);
    if (!arr) {
        printf("DEBUG xc_array_create_with_capacity: failed to allocate memory\n");
        return NULL;
    }
    
    /* 手动设置类型指针，确保它被正确设置 */
    if (XC_RUNTIME_EXT(rt)->array_type) {
        ((xc_object_t *)arr)->type = XC_RUNTIME_EXT(rt)->array_type;
        printf("DEBUG xc_array_create_with_capacity: manually set type pointer to %p\n", ((xc_object_t *)arr)->type);
    } else {
        printf("DEBUG xc_array_create_with_capacity: array_type is NULL, using static array_type\n");
        ((xc_object_t *)arr)->type = &array_type;
        printf("DEBUG xc_array_create_with_capacity: manually set type pointer to %p\n", ((xc_object_t *)arr)->type);
    }
    
    /* 确保对象头的类型ID也被正确设置 */
    xc_header_t* header = XC_HEADER(arr);
    if (header) {
        printf("DEBUG xc_array_create_with_capacity: header at %p, type is %d\n", header, header->type);
    } else {
        printf("DEBUG xc_array_create_with_capacity: header is NULL\n");
    }

    arr->items = NULL;
    arr->length = 0;
    arr->capacity = 0;

    if (initial_capacity > 0) {
        if (!array_ensure_capacity(arr, initial_capacity)) {
            printf("DEBUG xc_array_create_with_capacity: failed to ensure capacity\n");
            xc_gc_free(rt, (xc_object_t *)arr);
            return NULL;
        }
    }

    printf("DEBUG xc_array_create_with_capacity: returning array at %p\n", arr);
    return (xc_object_t *)arr;
}

/* Array operations */
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *obj) {
    assert(xc_is_array(rt, obj));
    xc_array_t *arr = (xc_array_t *)obj;
    return arr->length;
}

xc_object_t *xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;
    
    if (index >= array->length) {
        return NULL;
    }
    
    return array->items[index];
}

void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;

    if (index >= array->capacity && !array_ensure_capacity(array, index + 1)) {
        return;
    }

    if (array->items[index]) {
        xc_gc_free(rt, array->items[index]);
    }

    array->items[index] = value;
    if (value) {
        xc_gc_add_ref(rt, value);
    }

    if (index >= array->length) {
        array->length = index + 1;
    }
}

void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;
    
    if (!array_ensure_capacity(array, array->length + 1)) {
        return;
    }

    array->items[array->length] = value;
    if (value) {
        xc_gc_add_ref(rt, value);
    }
    array->length++;
}

xc_object_t *xc_array_pop(xc_runtime_t *rt, xc_object_t *arr) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;

    if (array->length == 0) {
        return NULL;
    }

    xc_object_t *value = array->items[array->length - 1];
    array->items[array->length - 1] = NULL;
    array->length--;

    /* Decrease reference count when removing from array */
    if (value) {
        xc_gc_release(rt, value);
    }

    return value;
}

/* Add element to the beginning of the array */
void xc_array_unshift(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;
    
    if (!array_ensure_capacity(array, array->length + 1)) {
        return;
    }
    
    /* Shift all elements right by one position */
    for (size_t i = array->length; i > 0; i--) {
        array->items[i] = array->items[i - 1];
    }
    
    array->items[0] = value;
    if (value) {
        xc_gc_add_ref(rt, value);
    }
    array->length++;
}

/* Remove and return the first element of the array */
xc_object_t *xc_array_shift(xc_runtime_t *rt, xc_object_t *arr) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;
    
    if (array->length == 0) {
        return NULL;
    }
    
    xc_object_t *value = array->items[0];
    
    /* Shift all elements left by one position */
    for (size_t i = 0; i < array->length - 1; i++) {
        array->items[i] = array->items[i + 1];
    }
    
    array->items[array->length - 1] = NULL;
    array->length--;
    
    /* Decrease reference count when removing from array */
    if (value) {
        xc_gc_release(rt, value);
    }
    
    return value;
}

/* Create a new array with elements from start to end (exclusive) */
xc_object_t *xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end) {
    assert(xc_is_array(rt, arr));
    xc_array_t *array = (xc_array_t *)arr;
    
    /* Convert negative indices (counting from end) */
    int length = (int)array->length;
    
    if (start < 0) {
        start = length + start;
    }
    
    if (end < 0) {
        end = length + end;
    }
    
    /* Clamp indices to valid range */
    if (start < 0) {
        start = 0;
    }
    
    if (end > length) {
        end = length;
    }
    
    /* Handle invalid range */
    if (start >= length || start >= end) {
        return xc_array_create(rt);
    }
    
    /* Create new array for the slice */
    size_t slice_length = end - start;
    xc_object_t *slice = xc_array_create_with_capacity(rt, slice_length);
    if (!slice) {
        return NULL;
    }
    
    /* Copy elements to the new array */
    for (int i = start; i < end; i++) {
        xc_array_push(rt, slice, array->items[i]);
    }
    
    return slice;
}

/* Concatenate two arrays into a new array */
xc_object_t *xc_array_concat(xc_runtime_t *rt, xc_object_t *arr1, xc_object_t *arr2) {
    assert(xc_is_array(rt, arr1));
    assert(xc_is_array(rt, arr2));
    
    xc_array_t *array1 = (xc_array_t *)arr1;
    xc_array_t *array2 = (xc_array_t *)arr2;
    
    /* Create new array with capacity for all elements */
    size_t total_length = array1->length + array2->length;
    xc_object_t *result = xc_array_create_with_capacity(rt, total_length);
    if (!result) {
        return NULL;
    }
    
    /* Copy elements from first array */
    for (size_t i = 0; i < array1->length; i++) {
        xc_array_push(rt, result, array1->items[i]);
    }
    
    /* Copy elements from second array */
    for (size_t i = 0; i < array2->length; i++) {
        xc_array_push(rt, result, array2->items[i]);
    }
    
    return result;
}

/* Find the index of a value in an array */
int xc_array_find_index(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    return xc_array_find_index_from(rt, arr, value, 0);
}

/* Find the index of a value in an array starting from a specific index */
int xc_array_find_index_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index) {
    if (!xc_is_array_object(rt, arr)) {
        return -1;
    }
    
    xc_array_t *array = (xc_array_t *)arr;
    size_t length = array->length;
    
    if (from_index < 0) {
        from_index = 0;
    }
    
    for (size_t i = from_index; i < length; i++) {
        if (xc_compare_objects(rt, array->items[i], value) == 0) {
            return (int)i;
        }
    }
    
    return -1;
}

/* Public index_of function that uses the internal find_index function */
int xc_array_index_of(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value) {
    return xc_array_find_index(rt, arr, value);
}

/* Public index_of_from function that uses the internal find_index_from function */
int xc_array_index_of_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index) {
    return xc_array_find_index_from(rt, arr, value, from_index);
}

/* Convert an object to a string representation */
static xc_object_t *xc_to_string_internal(xc_runtime_t *rt, xc_object_t *obj) {
    if (obj == NULL) {
        return xc_string_object_create(rt, "null");
    }
    
    if (xc_is_string_object(rt, obj)) {
        return xc_gc_retain(rt, obj);
    }
    
    if (xc_is_number_object(rt, obj)) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.14g", xc_number_get_value(rt, obj));
        return xc_string_object_create(rt, buffer);
    }
    
    if (xc_is_boolean_object(rt, obj)) {
        return xc_string_object_create(rt, xc_boolean_get_value(rt, obj) ? "true" : "false");
    }
    
    if (xc_is_null_object(rt, obj)) {
        return xc_string_object_create(rt, "null");
    }
    
    if (xc_is_array_object(rt, obj)) {
        return xc_array_join_elements(rt, obj, xc_string_object_create(rt, ","));
    }
    
    // Default for other types
    return xc_string_object_create(rt, "[object Object]");
}

/* Join array elements into a string */
xc_object_t *xc_array_join_elements(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator) {
    if (!xc_is_array_object(rt, arr)) {
        return xc_string_object_create(rt, "");
    }
    
    xc_array_t *array = (xc_array_t *)arr;
    size_t length = array->length;
    
    if (length == 0) {
        return xc_string_object_create(rt, "");
    }
    
    const char *sep_str = ",";
    if (separator != NULL && xc_is_string_object(rt, separator)) {
        sep_str = xc_string_get_value(rt, separator);
    }
    
    // Calculate the total length of the result string
    size_t total_length = 0;
    size_t sep_len = strlen(sep_str);
    
    for (size_t i = 0; i < length; i++) {
        xc_object_t *str = xc_to_string_internal(rt, array->items[i]);
        total_length += strlen(xc_string_get_value(rt, str));
        xc_gc_release(rt, str);
        
        if (i < length - 1) {
            total_length += sep_len;
        }
    }
    
    // Allocate memory for the result string
    char *result = (char *)malloc(total_length + 1);
    if (result == NULL) {
        return xc_string_object_create(rt, "");
    }
    
    // Build the result string
    char *p = result;
    for (size_t i = 0; i < length; i++) {
        xc_object_t *str = xc_to_string_internal(rt, array->items[i]);
        const char *str_value = xc_string_get_value(rt, str);
        size_t str_len = strlen(str_value);
        
        memcpy(p, str_value, str_len);
        p += str_len;
        
        xc_gc_release(rt, str);
        
        if (i < length - 1) {
            memcpy(p, sep_str, sep_len);
            p += sep_len;
        }
    }
    
    *p = '\0';
    
    xc_object_t *result_obj = xc_string_object_create(rt, result);
    free(result);
    
    return result_obj;
}

/* Public join function that uses the internal join_elements function */
xc_object_t *xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator) {
    return xc_array_join_elements(rt, arr, separator);
}

/* Type checking */
bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj) {
    // 检查对象是否为NULL
    if (!obj) return false;
    
    // 检查类型指针是否匹配
    if (obj->type == XC_RUNTIME_EXT(rt)->array_type) return true;
    
    // 检查类型ID是否匹配 (通过xc_header_t获取类型ID)
    xc_header_t* header = XC_HEADER(obj);
    if (header && header->type == XC_TYPE_ARRAY) return true;
    
    return false;
}