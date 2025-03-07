/*
 * xc_gc.h - XC garbage collector header file
 */
#ifndef XC_GC_H
#define XC_GC_H

#include "xc.h"
#include "xc_internal.h"

/* Garbage collector color marks for tri-color marking */
#define XC_GC_WHITE      0   /* Object is not reachable (candidate for collection) */
#define XC_GC_GRAY       1   /* Object is reachable but its children haven't been scanned */
#define XC_GC_BLACK      2   /* Object is reachable and its children have been scanned */
#define XC_GC_PERMANENT  3   /* Object is permanently reachable (never collected) */

/* GC configuration structure */
typedef struct xc_gc_config {
    size_t initial_heap_size;   /* Initial size of the heap in bytes */
    size_t max_heap_size;       /* Maximum size of the heap in bytes */
    double growth_factor;       /* Heap growth factor when resizing */
    double gc_threshold;        /* Memory usage threshold to trigger GC */
    size_t max_alloc_before_gc; /* Maximum number of allocations before forced GC */
} xc_gc_config_t;

/* Default GC configuration */
#define XC_GC_DEFAULT_CONFIG { \
    .initial_heap_size = 1024 * 1024, \
    .max_heap_size = 1024 * 1024 * 1024, \
    .growth_factor = 1.5, \
    .gc_threshold = 0.7, \
    .max_alloc_before_gc = 10000 \
}

/* GC statistics structure */
typedef struct xc_gc_stats {
    size_t heap_size;           /* Current heap size in bytes */
    size_t used_memory;         /* Used memory in bytes */
    size_t total_allocated;     /* Total allocated objects since start */
    size_t total_freed;         /* Total freed objects since start */
    size_t gc_cycles;           /* Number of GC cycles */
    double avg_pause_time_ms;   /* Average GC pause time in milliseconds */
    double last_pause_time_ms;  /* Last GC pause time in milliseconds */
} xc_gc_stats_t;

/* GC initialization and shutdown */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config);
void xc_gc_shutdown(xc_runtime_t *rt);

/* GC control functions */
void xc_gc_run(xc_runtime_t *rt);
void xc_gc_enable(xc_runtime_t *rt);
void xc_gc_disable(xc_runtime_t *rt);
bool xc_gc_is_enabled(xc_runtime_t *rt);

/* Memory management functions */
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id);
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj);  /* Mark an object as reachable */

/* Reference management */
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj);
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj);

/* Root set management */
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr);
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr);

/* GC statistics */
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt);
void xc_gc_print_stats(xc_runtime_t *rt);

void xc_gc_release_object(xc_val obj);

/**
 * Global release function for backward compatibility
 */
void xc_release(xc_val obj);

#endif /* XC_GC_H */
