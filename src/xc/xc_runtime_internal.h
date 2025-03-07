/*
 * xc_runtime_internal.h - XC internal runtime structures
 * This file contains internal implementation details of the runtime system
 */
#ifndef XC_RUNTIME_INTERNAL_H
#define XC_RUNTIME_INTERNAL_H

#include "xc.h"
#include "xc_object.h"

/* Forward declaration for exception frame */
typedef struct xc_exception_frame xc_exception_frame_t;

/* Cast runtime to extended runtime */
#define XC_RUNTIME_EXT(rt) ((xc_runtime_extended_t *)(rt))

/* 
 * Extended runtime structure with GC context
 * This extends the basic runtime structure with GC-specific fields
 */
typedef struct xc_runtime_extended {
    xc_runtime_t base;        /* Base runtime structure */
    void *gc_context;         /* Garbage collector context */
    xc_type_t *type_handlers[256]; /* Type handlers for different object types */
    xc_exception_frame_t *exception_frame; /* Current exception frame */
    
    /* Builtin types */
    xc_type_t *null_type;     /* Type for null objects */
    xc_type_t *boolean_type;  /* Type for boolean objects */
    xc_type_t *number_type;   /* Type for number objects */
    xc_type_t *string_type;   /* Type for string objects */
    xc_type_t *array_type;    /* Type for array objects */
    xc_type_t *object_type;   /* Type for generic objects */
    xc_type_t *function_type; /* Type for function objects */
    xc_type_t *error_type;    /* Type for error objects */
} xc_runtime_extended_t;

#endif /* XC_RUNTIME_INTERNAL_H */
