/*
 * xc_function.h - Function type definitions
 */
#ifndef XC_FUNCTION_H
#define XC_FUNCTION_H

#include "../xc.h"
#include "../xc_object.h"

/* Function object structure */
typedef struct xc_function_t {
    xc_object_t base;          /* Must be first */
    xc_function_ptr_t handler; /* Function handler */
    xc_object_t *closure;      /* Closure environment */
    xc_object_t *this_obj;     /* Bound this value */
} xc_function_t;

#endif /* XC_FUNCTION_H */