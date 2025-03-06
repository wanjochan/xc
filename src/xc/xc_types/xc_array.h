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

#endif /* XC_ARRAY_H */