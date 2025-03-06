/*
 * xc_array.h - Array type definitions
 */
#ifndef XC_ARRAY_H
#define XC_ARRAY_H

#include "../xc.h"
#include "../xc_object.h"

/* Array object structure */
typedef struct xc_array_t {
    xc_object_t base;     /* Must be first */
    xc_object_t **items;  /* Array of object pointers */
    size_t length;        /* Current number of items */
    size_t capacity;      /* Allocated capacity */
} xc_array_t;

/* Array type registration */
void xc_register_array_type(xc_runtime_t *rt);

/* Array creation functions */
xc_object_t *xc_array_create(xc_runtime_t *rt);
xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t initial_capacity);
xc_object_t *xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count);

/* Basic array operations */
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *obj);
xc_object_t *xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index);
void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value);

/* Stack operations (push/pop) */
void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_pop(xc_runtime_t *rt, xc_object_t *arr);

/* Queue operations (shift/unshift) */
void xc_array_unshift(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_shift(xc_runtime_t *rt, xc_object_t *arr);

/* Array manipulation */
xc_object_t *xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end);
xc_object_t *xc_array_concat(xc_runtime_t *rt, xc_object_t *arr1, xc_object_t *arr2);
xc_object_t *xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator);

/* Array searching */
int xc_array_index_of(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
int xc_array_index_of_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index);

/* Type checking */
bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj);

#endif /* XC_ARRAY_H */