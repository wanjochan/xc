/*
 * xc_types.h - XC type system header file
 */
#ifndef XC_TYPES_H
#define XC_TYPES_H

#include "../xc.h"
#include "../xc_object.h"

/*
 * Type IDs are defined in xc.h
 * We use those definitions instead of redefining them here
 */

/*
 * Type registration functions
 */
void xc_register_null_type(xc_runtime_t *rt);
void xc_register_boolean_type(xc_runtime_t *rt);
void xc_register_number_type(xc_runtime_t *rt);
void xc_register_string_type(xc_runtime_t *rt);
void xc_register_array_type(xc_runtime_t *rt);
void xc_register_object_type(xc_runtime_t *rt);
void xc_register_function_type(xc_runtime_t *rt);
void xc_register_error_type(xc_runtime_t *rt);

/*
 * Type creation functions
 */
xc_object_t *xc_null_create(xc_runtime_t *rt);
xc_object_t *xc_boolean_create(xc_runtime_t *rt, bool value);
xc_object_t *xc_number_create(xc_runtime_t *rt, double value);
xc_object_t *xc_string_create(xc_runtime_t *rt, const char *value);
xc_object_t *xc_string_create_len(xc_runtime_t *rt, const char *value, size_t len);
xc_object_t *xc_array_create(xc_runtime_t *rt);
xc_object_t *xc_array_create_with_capacity(xc_runtime_t *rt, size_t capacity);
xc_object_t *xc_array_create_with_values(xc_runtime_t *rt, xc_object_t **values, size_t count);
xc_object_t *xc_object_create(xc_runtime_t *rt);
xc_object_t *xc_function_create(xc_runtime_t *rt, xc_function_ptr_t fn, xc_object_t *closure);
xc_object_t *xc_function_get_closure(xc_runtime_t *rt, xc_object_t *func);

/*
 * Type conversion functions
 */
bool xc_to_boolean(xc_runtime_t *rt, xc_object_t *obj);
double xc_to_number(xc_runtime_t *rt, xc_object_t *obj);
const char *xc_to_string(xc_runtime_t *rt, xc_object_t *obj);

/*
 * Type checking functions
 */
bool xc_is_null(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_boolean(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_number(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_string(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_array(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_object(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_function(xc_runtime_t *rt, xc_object_t *obj);
bool xc_is_error(xc_runtime_t *rt, xc_object_t *obj);

/*
 * Type value access
 */
bool xc_boolean_value(xc_runtime_t *rt, xc_object_t *obj);
double xc_number_value(xc_runtime_t *rt, xc_object_t *obj);
const char *xc_string_value(xc_runtime_t *rt, xc_object_t *obj);
size_t xc_string_length(xc_runtime_t *rt, xc_object_t *obj);
size_t xc_array_length(xc_runtime_t *rt, xc_object_t *obj);
xc_object_t *xc_array_get(xc_runtime_t *rt, xc_object_t *arr, size_t index);
void xc_array_set(xc_runtime_t *rt, xc_object_t *arr, size_t index, xc_object_t *value);
void xc_array_push(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_pop(xc_runtime_t *rt, xc_object_t *arr);
void xc_array_unshift(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
xc_object_t *xc_array_shift(xc_runtime_t *rt, xc_object_t *arr);
xc_object_t *xc_array_slice(xc_runtime_t *rt, xc_object_t *arr, int start, int end);
xc_object_t *xc_array_concat(xc_runtime_t *rt, xc_object_t *arr1, xc_object_t *arr2);
xc_object_t *xc_array_join(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *separator);
int xc_array_index_of(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value);
int xc_array_index_of_from(xc_runtime_t *rt, xc_object_t *arr, xc_object_t *value, int from_index);
xc_object_t *xc_object_get(xc_runtime_t *rt, xc_object_t *obj, const char *key);
void xc_object_set(xc_runtime_t *rt, xc_object_t *obj, const char *key, xc_object_t *value);
bool xc_object_has(xc_runtime_t *rt, xc_object_t *obj, const char *key);
void xc_object_delete(xc_runtime_t *rt, xc_object_t *obj, const char *key);
xc_object_t *xc_function_call(xc_runtime_t *rt, xc_object_t *func, xc_object_t *this_obj, size_t argc, xc_object_t **argv);

/*
 * Type iteration
 */
void xc_array_foreach(xc_runtime_t *rt, xc_object_t *arr, void (*callback)(xc_runtime_t *rt, size_t index, xc_object_t *value, void *user_data), void *user_data);
void xc_object_foreach(xc_runtime_t *rt, xc_object_t *obj, void (*callback)(xc_runtime_t *rt, const char *key, xc_object_t *value, void *user_data), void *user_data);

/*
 * Type comparison
 */
bool xc_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
bool xc_strict_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);
int xc_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);

#endif /* XC_TYPES_H */
