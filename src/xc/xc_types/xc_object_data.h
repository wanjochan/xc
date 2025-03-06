/*
 * xc_object_data.h - Object data type definitions
 */
#ifndef XC_OBJECT_DATA_H
#define XC_OBJECT_DATA_H

#include "../xc.h"
#include "../xc_object.h"

/* Object property structure */
typedef struct xc_property {
    xc_object_t *key;    /* String key */
    xc_object_t *value;  /* Any value */
} xc_property_t;

/* Object data structure */
typedef struct xc_object_data_t {
    xc_object_t base;          /* Must be first */
    xc_property_t *properties; /* Array of properties */
    size_t count;             /* Number of properties */
    size_t capacity;          /* Allocated capacity */
    xc_object_t *prototype;   /* Prototype object */
} xc_object_data_t;

#endif /* XC_OBJECT_DATA_H */