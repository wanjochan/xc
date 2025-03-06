/*
 * xc_compare.c - XC type comparison functions
 */

#include "../xc.h"
#include "../xc_object.h"
#include "xc_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 
 * Compare two XC objects for equality
 * Returns true if objects are equal, false otherwise
 */
bool xc_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    /* NULL check */
    if (!a && !b) return true;
    if (!a || !b) return false;
    
    /* Same object check */
    if (a == b) return true;
    
    /* Type check */
    if (a->type != b->type) return false;
    
    /* Delegate to type-specific equal function */
    if (a->type && a->type->equal) {
        return a->type->equal(rt, a, b);
    }
    
    /* Default to pointer comparison */
    return a == b;
}

/*
 * Compare two XC objects for ordering
 * Returns -1 if a < b, 0 if a == b, 1 if a > b
 */
int xc_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    /* NULL check */
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    
    /* Same object check */
    if (a == b) return 0;
    
    /* Type check - different types are ordered by type ID */
    if (a->type != b->type) {
        return (a->type < b->type) ? -1 : 1;
    }
    
    /* Delegate to type-specific compare function */
    if (a->type && a->type->compare) {
        return a->type->compare(rt, a, b);
    }
    
    /* Default to pointer comparison */
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

/*
 * Strict equality check (same type and value)
 */
bool xc_strict_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    /* NULL check */
    if (!a && !b) return true;
    if (!a || !b) return false;
    
    /* Type check */
    if (a->type != b->type) return false;
    
    /* Delegate to type-specific equal function */
    if (a->type && a->type->equal) {
        return a->type->equal(rt, a, b);
    }
    
    /* Default to pointer comparison */
    return a == b;
}

/* 
 * Function to invoke a function object
 * This is a stub implementation that just returns NULL
 * The real implementation would be in xc_function.c
 */
xc_val xc_function_invoke(xc_val func, xc_val this_obj, int argc, xc_val* argv) {
    /* This is a stub implementation */
    return NULL;
} 