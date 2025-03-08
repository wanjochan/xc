#include "../xc.h"
#include "../xc_internal.h"

static xc_runtime_t* rt = NULL;
/* Forward declarations */
static bool array_ensure_capacity(xc_array_t *arr, size_t needed);
static xc_object_t *xc_to_string_internal(xc_runtime_t *rt, xc_object_t *obj);
static xc_object_t *xc_array_join_elements(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator);
int xc_array_find_index_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index);
static void array_mark(xc_object_t *obj, mark_func mark);
static void array_free(xc_runtime_t *rt, xc_object_t *obj);
static bool array_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static int array_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
static xc_val array_creator(int type, va_list args);
static void array_initializer(void);

/* 值访问和类型转换函数 */
static void* array_get_value(xc_val obj);
static xc_val array_convert_to(xc_val obj, int target_type);

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

/* Array methods */
static void array_mark(xc_object_t *obj, mark_func mark) {
    xc_array_t *arr = (xc_array_t *)obj;
    for (size_t i = 0; i < arr->length; i++) {
        if (arr->items[i]) {
            /* Mark each item in the array */
            mark(arr->items[i]);
        }
    }
}

static void array_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_array_t *arr = (xc_array_t *)obj;
    /* Release all items */
    for (size_t i = 0; i < arr->length; i++) {
        if (arr->items[i]) {
            //xc_gc_free(rt, arr->items[i]);
printf("TODO xc_gc_free or memmove??");
        }
    }
    /* Free the items array */
    free(arr->items);//??? not using gc??
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

/* Type descriptor for array type */
static xc_type_lifecycle_t array_type = {
    .initializer = NULL,
    .cleaner = NULL,
    .creator = array_creator,
    .destroyer = (xc_destroy_func)array_free,
    .marker = array_mark,
    // .allocator = NULL,
    .name = "array",
    .equal = (bool (*)(xc_val, xc_val))array_equal,
    .compare = (int (*)(xc_val, xc_val))array_compare,
    .flags = 0
};

/* Array creator function for type system */
static xc_val array_creator(int type, va_list args) {
    // // 使用全局运行时实例
    // xc_runtime_t *rt = &xc;
    // //printf("DEBUG array_creator called, type=%d\n", type);
    // // 创建一个空数组
    return xc_array_create(rt);
}

/* 方法包装函数 */
static xc_val array_length_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_length_method called, self=%p\n", self);
    return (xc_val)xc_array_length(rt, (xc_object_t *)self);
}

static xc_val array_get_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_get_method called, self=%p, arg=%p\n", self, arg);
    // 参数应该是一个数字，表示索引
    if (!arg || !rt->is(arg, XC_TYPE_NUMBER)) {
        //printf("DEBUG array_get_method: arg is not a number\n");
        return NULL;
    }
    // 获取索引值
    long index = (long)arg;
    //printf("DEBUG array_get_method: index=%ld\n", index);
    return xc_array_get(rt, (xc_object_t *)self, index);
}

static xc_val array_push_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_push_method called, self=%p, arg=%p\n", self, arg);
    xc_array_push(rt, (xc_object_t *)self, (xc_object_t *)arg);
    return self; // 返回数组自身
}

static xc_val array_pop_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_pop_method called, self=%p\n", self);
    return xc_array_pop(rt, (xc_object_t *)self);
}

static xc_val array_slice_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_slice_method called, self=%p, arg=%p\n", self, arg);
    
    // 参数应该是一个数组，包含起始和结束索引
    if (!arg || !rt->is(arg, XC_TYPE_ARRAY)) {
        //printf("DEBUG array_slice_method: arg is not an array\n");
        return NULL;
    }
    
    // 获取起始索引
    xc_val start_val = xc_array_get(rt, (xc_object_t *)arg, 0);
    if (!start_val || !rt->is(start_val, XC_TYPE_NUMBER)) {
        //printf("DEBUG array_slice_method: start index is not a number\n");
        return NULL;
    }
    
    // 获取结束索引
    xc_val end_val = xc_array_get(rt, (xc_object_t *)arg, 1);
    if (!end_val || !rt->is(end_val, XC_TYPE_NUMBER)) {
        //printf("DEBUG array_slice_method: end index is not a number\n");
        return NULL;
    }
    
    int start = (int)xc_number_value(rt, (xc_object_t *)start_val);
    int end = (int)xc_number_value(rt, (xc_object_t *)end_val);
    
    //printf("DEBUG array_slice_method: start=%d, end=%d\n", start, end);
    
    return xc_array_slice(rt, (xc_object_t *)self, start, end);
}

static xc_val array_concat_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_concat_method called, self=%p, arg=%p\n", self, arg);
    
    // 参数应该是一个数组
    if (!arg || !rt->is(arg, XC_TYPE_ARRAY)) {
        //printf("DEBUG array_concat_method: arg is not an array\n");
        return NULL;
    }
    
    return xc_array_concat(rt, (xc_object_t *)self, (xc_object_t *)arg);
}

static xc_val array_join_method(xc_val self, xc_val arg) {
    // xc_runtime_t *rt = &xc;
    //printf("DEBUG array_join_method called, self=%p, arg=%p\n", self, arg);
    
    // 参数应该是一个字符串，表示分隔符
    if (!arg || !rt->is(arg, XC_TYPE_STRING)) {
        //printf("DEBUG array_join_method: arg is not a string\n");
        return NULL;
    }
    
    return xc_array_join_elements(rt, (xc_object_t *)self, (xc_object_t *)arg);
}

/* Array initializer function for type system */
static void array_initializer() {
    // xc_runtime_t *rt = &xc;
    // 数组类型的初始化逻辑
    //printf("DEBUG array_initializer called\n");
    //printf("DEBUG array_initializer: registering methods for type %d\n", XC_TYPE_ARRAY);
    //printf("DEBUG array_initializer: length method at %p\n", array_length_method);
    //printf("DEBUG array_initializer: get method at %p\n", array_get_method);
    //printf("DEBUG array_initializer: push method at %p\n", array_push_method);
    //printf("DEBUG array_initializer: pop method at %p\n", array_pop_method);
    //printf("DEBUG array_initializer: slice method at %p\n", array_slice_method);
    //printf("DEBUG array_initializer: concat method at %p\n", array_concat_method);
    //printf("DEBUG array_initializer: join method at %p\n", array_join_method);
    
    /* 注册数组方法 */
    rt->register_method(XC_TYPE_ARRAY, "length", array_length_method);
    rt->register_method(XC_TYPE_ARRAY, "get", array_get_method);
    rt->register_method(XC_TYPE_ARRAY, "push", array_push_method);
    rt->register_method(XC_TYPE_ARRAY, "pop", array_pop_method);
    rt->register_method(XC_TYPE_ARRAY, "slice", array_slice_method);
    rt->register_method(XC_TYPE_ARRAY, "concat", array_concat_method);
    rt->register_method(XC_TYPE_ARRAY, "join", array_join_method);
    
    //printf("DEBUG array_initializer: methods registered\n");
}

/* Register array type */
void xc_register_array_type(xc_runtime_t *caller_rt) {
    rt = caller_rt;
    //printf("DEBUG xc_register_array_type: registering array type\n");
    
    /* 注册类型 */
    int type_id = rt->register_type("array", &array_type);
    
    /* 使用 XC_RUNTIME_EXT 宏访问扩展运行时结构体 */
    xc_array_type = &array_type;
    //printf("DEBUG xc_register_array_type: set array_type to %p\n", xc_array_type);
    
    /* 调用初始化函数注册数组方法 */
    array_initializer();

    // 设置生命周期函数
    array_type.initializer = (xc_initializer_func)array_initializer;
    array_type.cleaner = NULL;
    array_type.creator = array_creator;
    array_type.destroyer = (xc_destroy_func)array_free;
    array_type.marker = (xc_marker_func)array_mark;
    array_type.name = "array";
    array_type.equal = (void*)array_equal;
    array_type.compare = (void*)array_compare;
    
    // 新增：值访问和类型转换
    array_type.get_value = array_get_value;
    array_type.convert_to = array_convert_to;
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
    //printf("DEBUG xc_array_create called\n");
    return xc_array_create_with_capacity(rt, 0);
}

/* Create array with initial values */
xc_object_t *xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count) {
    //printf("DEBUG xc_array_create_with_values called, count=%zu\n", count);
    xc_object_t *arr = xc_array_create_with_capacity(rt, count);
    if (!arr) {
        //printf("DEBUG xc_array_create_with_values: failed to create array\n");
        return NULL;
    }
    
    for (size_t i = 0; i < count; i++) {
        xc_array_push(rt, arr, values[i]);
    }
    
    return arr;
}

xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t capacity) {
    //printf("DEBUG xc_array_create_with_capacity called, capacity=%zu\n", capacity);
    //printf("DEBUG xc_array_create_with_capacity: rt=%p\n", rt);
    //printf("DEBUG xc_array_create_with_capacity: array_type=%p\n", xc_array_type);
    
    /* 分配内存 */
    xc_array_t *arr = (xc_array_t *)xc_gc_alloc(rt, sizeof(xc_array_t), XC_TYPE_ARRAY);
    if (!arr) {
        //printf("DEBUG xc_array_create_with_capacity: failed to allocate memory\n");
        return NULL;
    }
    
    /* 初始化对象 */
    ((xc_object_t *)arr)->type_id = XC_TYPE_ARRAY;
    arr->length = 0;
    arr->capacity = capacity;
    
    /* 分配数组内存 */
    if (capacity > 0) {
        arr->items = (xc_object_t **)malloc(sizeof(xc_object_t *) * capacity);
        if (!arr->items) {
            //xc_gc_free(rt, (xc_object_t *)arr);
//TODO rt->delete(arr);
            return NULL;
        }
        memset(arr->items, 0, sizeof(xc_object_t *) * capacity);
    } else {
        arr->items = NULL;
    }
    
    //printf("DEBUG xc_array_create_with_capacity: returning array at %p\n", arr);
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

    // /* 如果当前位置有值，先减少引用计数 */
    // if (array->items[index]) {
    //     /* 减少引用计数，但不释放对象本身 */
    //     xc_gc_release(rt, array->items[index]);
    // }

    array->items[index] = value;
    // if (value) {
    //     /* 增加引用计数 */
    //     xc_gc_add_ref(rt, value);
    // }

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
    // if (value) {
    //     xc_gc_add_ref(rt, value);
    // }
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

    /* 不在这里减少引用计数，因为我们要返回这个对象 */
    /* 调用者负责在使用完毕后释放 */
    
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
    // if (value) {
    //     xc_gc_add_ref(rt, value);
    // }
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
    
    /* 不在这里减少引用计数，因为我们要返回这个对象 */
    /* 调用者负责在使用完毕后释放 */
    
    return value;
}

/* Create a new array with elements from start to end (exclusive) */
xc_object_t *xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end) {
    assert(xc_is_array(rt, arr));
    
    xc_array_t *array = (xc_array_t *)arr;
    size_t length = array->length;
    
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
        // 增加引用计数，因为我们要将对象添加到新数组中
        //xc_gc_retain(rt, array->items[i]);
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
        // 增加引用计数，因为我们要将对象添加到新数组中
        //xc_gc_retain(rt, array1->items[i]);
        xc_array_push(rt, result, array1->items[i]);
    }
    
    /* Copy elements from second array */
    for (size_t i = 0; i < array2->length; i++) {
        // 增加引用计数，因为我们要将对象添加到新数组中
        //xc_gc_retain(rt, array2->items[i]);
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
        return xc_string_create(rt, "null");
    }
    
    if (xc_is_string(rt, obj)) {
        //return xc_gc_retain(rt, obj);
        return obj;
    }
    
    if (xc_is_number(rt, obj)) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.14g", xc_number_value(rt, obj));
        return xc_string_create(rt, buffer);
    }
    
    if (xc_is_boolean(rt, obj)) {
        return xc_string_create(rt, xc_boolean_value(rt, obj) ? "true" : "false");
    }
    
    if (xc_is_null(rt, obj)) {
        return xc_string_create(rt, "null");
    }
    
    if (xc_is_array(rt, obj)) {
        // 为避免无限递归，我们使用简单的表示方法
        xc_array_t *array = (xc_array_t *)obj;
        if (array->length == 0) {
            return xc_string_create(rt, "[]");
        } else {
            // 使用 join 函数，但传递一个简单的分隔符
            // 注意：xc_array_join_elements 函数已经处理了循环引用的情况
            return xc_array_join_elements(rt, obj, xc_string_create(rt, ","));
        }
    }
    
    // Default for other types
    return xc_string_create(rt, "[object Object]");
}

/* Join array elements into a string */
xc_object_t *xc_array_join_elements(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator) {
    if (!xc_is_array(rt, arr)) {
        return xc_string_create(rt, "");
    }
    
    xc_array_t *array = (xc_array_t *)arr;
    size_t length = array->length;
    
    if (length == 0) {
        return xc_string_create(rt, "");
    }
    
    const char *sep_str = ",";
    if (separator != NULL && xc_is_string(rt, separator)) {
        sep_str = xc_string_value(rt, separator);
    }
    
    // 首先将所有元素转换为字符串并存储，避免重复转换
    xc_object_t **str_items = (xc_object_t **)malloc(length * sizeof(xc_object_t *));
    if (str_items == NULL) {
        return xc_string_create(rt, "");
    }
    
    // 计算结果字符串的总长度
    size_t total_length = 0;
    size_t sep_len = strlen(sep_str);
    
    for (size_t i = 0; i < length; i++) {
        // 避免无限递归：如果元素是数组，则用特殊表示
        if (xc_is_array(rt, array->items[i]) && array->items[i] == arr) {
            str_items[i] = xc_string_create(rt, "[Circular]");
        } else {
            str_items[i] = xc_to_string_internal(rt, array->items[i]);
        }
        
        total_length += strlen(xc_string_value(rt, str_items[i]));
        
        if (i < length - 1) {
            total_length += sep_len;
        }
    }
    
    // 分配结果字符串的内存
    char *result = (char *)malloc(total_length + 1);
    if (result == NULL) {
        // // 释放临时字符串
        // for (size_t i = 0; i < length; i++) {
        //     xc_gc_release(rt, str_items[i]);
        // }
        free(str_items);
        return xc_string_create(rt, "");
    }
    
    // 构建结果字符串
    char *p = result;
    for (size_t i = 0; i < length; i++) {
        const char *str_value = xc_string_value(rt, str_items[i]);
        size_t str_len = strlen(str_value);
        
        memcpy(p, str_value, str_len);
        p += str_len;
        
        // // 释放临时字符串
        // xc_gc_release(rt, str_items[i]);
        
        if (i < length - 1) {
            memcpy(p, sep_str, sep_len);
            p += sep_len;
        }
    }
    
    *p = '\0';
    
    // 释放临时数组
    free(str_items);
    
    xc_object_t *result_obj = xc_string_create(rt, result);
    free(result);
    
    return result_obj;
}

/* Public join function that uses the internal join_elements function */
xc_object_t *xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator) {
    return xc_array_join_elements(rt, arr, separator);
}

/* Type checking */
bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type_id == XC_TYPE_ARRAY;
}

/* 获取数组值（返回长度） */
static void* array_get_value(xc_val obj) {
    xc_array_t* array = (xc_array_t*)obj;
    // 返回指向长度的指针（注意：这里需要静态存储）
    static size_t length;
    length = array->length;
    return &length;
}

/* 转换到其他类型 */
static xc_val array_convert_to(xc_val obj, int target_type) {
    xc_array_t* array = (xc_array_t*)obj;
    
    switch (target_type) {
        case XC_TYPE_BOOL:
            // 非空数组为true
            return rt->new(XC_TYPE_BOOL, array->length > 0);
            
        case XC_TYPE_NUMBER:
            // 返回数组长度
            return rt->new(XC_TYPE_NUMBER, (double)array->length);
            
        case XC_TYPE_STRING: {
            // 将数组转换为字符串表示
            // 这里简化处理，返回类似 "[object Array]" 的字符串
            return rt->new(XC_TYPE_STRING, "[object Array]");
        }
            
        case XC_TYPE_ARRAY:
            return obj; // 已经是数组类型
            
        default:
            return NULL; // 不支持的转换
    }
}
