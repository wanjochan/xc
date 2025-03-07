/*
 * xc_object.h - XC object structure definition
 */
#ifndef XC_OBJECT_H
#define XC_OBJECT_H

#include "xc.h"

/* 
 * Function pointer type for XC functions
 */
typedef xc_val (*xc_function_ptr_t)(xc_runtime_t *rt, xc_val this_obj, int argc, xc_val *argv);

/* 
 * Basic object structure for all XC objects
 * This is the common header for all objects managed by the GC
 */
typedef struct xc_object {
    size_t size;              /* Total size of the object in bytes */
    struct xc_type *type;     /* Pointer to type descriptor */
    int ref_count;            /* Reference count for manual memory management */
    int gc_color;             /* GC mark color (white, gray, black, permanent) */
    struct xc_object *gc_next; /* Next object in the GC list */
    /* Object data follows this header */
} xc_object_t;

/* Type flags */
#define XC_TYPE_PRIMITIVE  0x0001  /* Primitive type (number, string, etc) */
#define XC_TYPE_COMPOSITE 0x0002   /* Composite type (array, object) */
#define XC_TYPE_CALLABLE  0x0004   /* Callable type (function) */
#define XC_TYPE_INTERNAL  0x0008   /* Internal type */

/*
 * Type handler structure
 * Contains function pointers for type-specific operations
 */
typedef struct xc_type {
    const char *name;         /* Type name */
    int flags;               /* Type flags */
    void (*free)(xc_runtime_t *rt, xc_object_t *obj);  /* Free type-specific resources */
    void (*mark)(xc_runtime_t *rt, xc_object_t *obj);  /* Mark referenced objects */
    bool (*equal)(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);  /* Equality comparison */
    int (*compare)(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b);  /* Ordering comparison */
} xc_type_t;

/* Forward declaration for exception frame - kept for backward compatibility */
typedef struct xc_exception_frame xc_exception_frame_t;

/* Forward declaration of extended runtime */
typedef struct xc_runtime_extended xc_runtime_extended_t;

#endif /* XC_OBJECT_H */ 