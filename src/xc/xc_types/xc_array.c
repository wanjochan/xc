/*
 * xc_array.c - Array type implementation
 */

#include "../xc.h"
#include "../xc_object.h"
#include "../xc_gc.h"
#include "xc_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Array object structure */
typedef struct {
    xc_object_t base;     /* Must be first */
    xc_object_t **items;  /* Array of object pointers */
    size_t length;        /* Current number of items */
    size_t capacity;      /* Allocated capacity */
} xc_array_t;

/* Forward declarations */
static bool array_ensure_capacity(xc_array_t *arr, size_t needed);

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

/* Type descriptor for array type */
static xc_type_t array_type = {
    .name = "array",
    .flags = XC_TYPE_COMPOSITE,
    .mark = array_mark,
    .free = array_free,
    .equal = array_equal,
    .compare = array_compare
};

/* Register array type */
void xc_register_array_type(xc_runtime_t *rt) {
    XC_RUNTIME_EXT(rt)->array_type = &array_type;
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
    return xc_array_create_with_capacity(rt, 0);
}

xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t initial_capacity) {
    /* 使用 xc_gc_alloc 分配对象，并传递类型索引 */
    xc_array_t *arr = (xc_array_t *)xc_gc_alloc(rt, sizeof(xc_array_t), XC_TYPE_ARRAY);
    if (!arr) {
        return NULL;
    }
    
    /* 设置正确的类型指针 */
    ((xc_object_t *)arr)->type = XC_RUNTIME_EXT(rt)->array_type;

    arr->items = NULL;
    arr->length = 0;
    arr->capacity = 0;

    if (initial_capacity > 0) {
        if (!array_ensure_capacity(arr, initial_capacity)) {
            xc_gc_free(rt, (xc_object_t *)arr);
            return NULL;
        }
    }

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

    return value;
}

/* Type checking */
bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj) {
    return obj && obj->type == XC_RUNTIME_EXT(rt)->array_type;
}