/*
 * xc_gc.c - XC garbage collector implementation
 */
#include "xc.h"
#include "xc_gc.h"
#include "xc_internal.h"

/* Forward declarations of internal type structures */
typedef struct xc_array_t xc_array_t;
typedef struct xc_object_data_t xc_object_data_t;
typedef struct xc_function_t xc_function_t;

/* Internal GC context structure */
typedef struct xc_gc_context {
    xc_gc_config_t config;           /* GC configuration */
    size_t heap_size;                /* Current heap size in bytes */
    size_t used_memory;              /* Used memory in bytes */
    size_t allocation_count;         /* Allocations since last GC */
    size_t total_allocated;          /* Total allocated objects */
    size_t total_freed;              /* Total freed objects */
    size_t gc_cycles;                /* Number of GC cycles */
    double total_pause_time_ms;      /* Total GC pause time in ms */
    bool enabled;                    /* Whether GC is enabled */
    
    /* Root set */
    xc_object_t ***roots;            /* Array of pointers to root objects */
    size_t root_count;               /* Number of roots */
    size_t root_capacity;            /* Capacity of roots array */
    
    /* Object lists for tri-color marking */
    xc_object_t *white_list;         /* White objects (candidates for collection) */
    xc_object_t *gray_list;          /* Gray objects (reachable but not scanned) */
    xc_object_t *black_list;         /* Black objects (reachable and scanned) */
} xc_gc_context_t;

/* Get GC context from runtime */
static xc_gc_context_t *xc_gc_get_context(xc_runtime_t *rt) {
    xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;
    return (xc_gc_context_t *)ext_rt->gc_context;
}

/* Initialize the garbage collector */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config) {
    xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;
    
    /* Allocate and initialize GC context */
    xc_gc_context_t *gc = (xc_gc_context_t *)malloc(sizeof(xc_gc_context_t));
    if (!gc) {
        fprintf(stderr, "Failed to allocate GC context\n");
        exit(1);
    }
    
    /* Copy configuration or use defaults */
    if (config) {
        memcpy(&gc->config, config, sizeof(xc_gc_config_t));
    } else {
        xc_gc_config_t default_config = XC_GC_DEFAULT_CONFIG;
        memcpy(&gc->config, &default_config, sizeof(xc_gc_config_t));
    }
    
    /* Initialize context fields */
    gc->heap_size = 0;
    gc->used_memory = 0;
    gc->allocation_count = 0;
    gc->total_allocated = 0;
    gc->total_freed = 0;
    gc->gc_cycles = 0;
    gc->total_pause_time_ms = 0;
    gc->enabled = true;
    
    /* Initialize root set */
    gc->roots = NULL;
    gc->root_count = 0;
    gc->root_capacity = 0;
    
    /* Initialize object lists */
    gc->white_list = NULL;
    gc->gray_list = NULL;
    gc->black_list = NULL;
    
    /* Store context in runtime */
    ext_rt->gc_context = gc;
}

/* Shutdown the garbage collector */
void xc_gc_shutdown(xc_runtime_t *rt) {
    xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Run a final GC to free all objects */
    xc_gc_run(rt);
    
    /* Free roots array */
    free(gc->roots);
    
    /* Free GC context */
    free(gc);
    ext_rt->gc_context = NULL;
}

/* Mark phase of GC - traverse object and mark all reachable objects */
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    
    /* Object is already marked, skip it */
    if (obj->gc_color == XC_GC_GRAY || obj->gc_color == XC_GC_BLACK) return;
    
    /* Mark object as gray (reachable but children not scanned) */
    obj->gc_color = XC_GC_GRAY;
    
    /* Add to gray list for processing */
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    obj->gc_next = gc->gray_list;
    gc->gray_list = obj;
}

/* Process gray list and mark all reachable objects */
static void xc_gc_process_gray_list(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Process all objects in the gray list */
    while (gc->gray_list) {
        /* Get and remove the first gray object */
        xc_object_t *obj = gc->gray_list;
        gc->gray_list = obj->gc_next;
        
        /* Mark object as black (fully processed) */
        obj->gc_color = XC_GC_BLACK;
        
        /* Add to black list */
        obj->gc_next = gc->black_list;
        gc->black_list = obj;
        
        /* Get extended runtime */
        xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;

        /* Call type-specific mark function if provided */
        if (obj->type && obj->type->mark) {
            obj->type->mark(rt, obj);
        } else {
            /* Default marking behavior for objects without type-specific mark function */
            if (obj->type == ext_rt->array_type) {
                /* Mark array elements */
                xc_array_t *arr = (xc_array_t *)obj;
                for (size_t i = 0; i < arr->length; i++) {
                    if (arr->items[i]) {
                        xc_gc_mark(rt, arr->items[i]);
                    }
                }
            } else if (obj->type == ext_rt->object_type) {
                /* Mark object properties */
                xc_object_data_t *object = (xc_object_data_t *)obj;
                for (size_t i = 0; i < object->count; i++) {
                    if (object->properties[i].key) {
                        xc_gc_mark(rt, object->properties[i].key);
                    }
                    if (object->properties[i].value) {
                        xc_gc_mark(rt, object->properties[i].value);
                    }
                }
                /* Mark prototype */
                if (object->prototype) {
                    xc_gc_mark(rt, object->prototype);
                }
            } else if (obj->type == ext_rt->function_type) {
                /* Mark function's closure variables */
                xc_function_t *func = (xc_function_t *)obj;
                if (func->closure) {
                    xc_gc_mark(rt, func->closure);
                }
                if (func->this_obj) {
                    xc_gc_mark(rt, func->this_obj);
                }
            }
        }
    }
}

/* Sweep phase of GC - detect and free unreachable objects */
static size_t xc_gc_sweep(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    size_t freed_count = 0;
    
    /* First pass: find objects with only internal references */
    xc_object_t *curr = gc->white_list;
    while (curr) {
        if (curr->gc_color == XC_GC_WHITE && curr->ref_count > 0) {
            /* Object has external references, skip it */
            curr->gc_color = XC_GC_BLACK;
        }
        curr = curr->gc_next;
    }
    
    /* Second pass: process white objects (unreachable or only internal refs) */
    xc_object_t *prev = NULL;
    curr = gc->white_list;
    
    while (curr) {
        xc_object_t *next = curr->gc_next;
        
        /* Skip objects that were marked or have external references */
        if (curr->gc_color != XC_GC_WHITE) {
            prev = curr;
            curr = next;
            continue;
        }
        
        /* Object is either unreachable or part of a cycle */
        
        /* Call type-specific cleanup if available */
        if (curr->type && curr->type->free) {
            curr->type->free(rt, curr);
        }
        
        /* Update pointers in the white list */
        if (prev) {
            prev->gc_next = next;
        } else {
            gc->white_list = next;
        }
        
        /* Update statistics */
        gc->used_memory -= curr->size;
        gc->total_freed++;
        freed_count++;
        
        /* Clear any remaining references */
        curr->ref_count = 0;
        
        /* Free the memory */
        free(curr);
        
        /* Move to next object */
        curr = next;
    }
    
    return freed_count;
}

/* Mark roots and process object graph */
static void xc_gc_mark_roots(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Mark all roots */
    for (size_t i = 0; i < gc->root_count; i++) {
        xc_object_t *root = *gc->roots[i];
        if (root) {
            xc_gc_mark(rt, root);
        }
    }
    
    /* Process all reachable objects */
    xc_gc_process_gray_list(rt);
}

/* Reset GC colors for next cycle */
static void xc_gc_reset_colors(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Move all black objects back to white list */
    while (gc->black_list) {
        xc_object_t *obj = gc->black_list;
        gc->black_list = obj->gc_next;
        
        /* Skip permanent objects */
        if (obj->gc_color == XC_GC_PERMANENT) continue;
        
        /* Mark as white for next cycle */
        obj->gc_color = XC_GC_WHITE;
        obj->gc_next = gc->white_list;
        gc->white_list = obj;
    }
}

/* Run a garbage collection cycle */
void xc_gc_run(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Skip if GC is disabled */
    if (!gc->enabled) return;
    
    /* Measure GC pause time */
    clock_t start_time = clock();
    
    /* Reset colors for multi-colored objects */
    xc_gc_reset_colors(rt);
    
    /* Mark all reachable objects */
    xc_gc_mark_roots(rt);
    
    /* Sweep unreachable objects */
    size_t freed_count = xc_gc_sweep(rt);
    
    /* Calculate pause time */
    clock_t end_time = clock();
    double pause_time_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    
    /* Update statistics */
    gc->gc_cycles++;
    gc->total_pause_time_ms += pause_time_ms;
    gc->allocation_count = 0;
    
    /* Print GC info if verbose mode is enabled */
    printf("GC: Freed %zu objects, %zu bytes recovered (%.2f ms)\n", 
           freed_count, gc->used_memory, pause_time_ms);
}

/* Allocate a new object */
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Check if we need to run GC */
    if (gc->enabled) {
        bool should_gc = false;
        
        /* Calculate memory pressure */
        double memory_pressure = (double)(gc->used_memory + size) / gc->heap_size;
        
        /* Adjust collection threshold based on GC effectiveness */
        size_t threshold = gc->config.max_alloc_before_gc;
        if (gc->gc_cycles > 0) {
            double last_gc_effectiveness = (double)gc->total_freed / gc->total_allocated;
            if (last_gc_effectiveness < 0.1) {
                /* Less than 10% freed, increase threshold to avoid frequent GCs */
                threshold = (size_t)(threshold * 1.5);
            } else if (last_gc_effectiveness > 0.5) {
                /* More than 50% freed, decrease threshold for more frequent GCs */
                threshold = (size_t)(threshold * 0.8);
            }
        }

        /* Check allocation threshold */
        if (gc->allocation_count >= threshold) {
            should_gc = true;
        }

        /* Handle different memory pressure scenarios */
        if (memory_pressure > gc->config.gc_threshold) {
            /* Medium pressure: normal GC */
            should_gc = true;
        }
        
        if (memory_pressure > 0.9) {
            /* High pressure: immediate GC and possible heap expansion */
            xc_gc_run(rt);
            
            /* Check if we still need more space after GC */
            double post_gc_pressure = (double)(gc->used_memory + size) / gc->heap_size;
            if (post_gc_pressure > 0.9) {
                /* Try to expand heap */
                size_t new_heap_size = (size_t)(gc->heap_size * gc->config.growth_factor);
                if (new_heap_size <= gc->config.max_heap_size) {
                    gc->heap_size = new_heap_size;
                }
            }
            should_gc = false; /* Already performed GC */
        } else if (memory_pressure > 0.95) {
            /* Critical pressure: aggressive GC */
            xc_gc_run(rt);
            if ((double)(gc->used_memory + size) / gc->heap_size > 0.95) {
                /* If still critical after GC, try one last heap expansion */
                size_t new_heap_size = gc->config.max_heap_size;
                if (gc->heap_size < new_heap_size) {
                    gc->heap_size = new_heap_size;
                }
            }
            should_gc = false; /* Already performed GC */
        }
        
        /* Run GC if needed and not already performed */
        if (should_gc) {
            xc_gc_run(rt);
        }
    }
    
    /* Allocate memory for the object */
    xc_object_t *obj = (xc_object_t *)malloc(size);
    if (!obj) {
        /* Memory allocation failed, force GC and try again */
        xc_gc_run(rt);
        obj = (xc_object_t *)malloc(size);
        if (!obj) {
            /* Still failed, report error */
            fprintf(stderr, "Memory allocation failed: size %zu\n", size);
            return NULL;
        }
    }
    
    /* Initialize basic object fields */
    obj->size = size;
    xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;
    obj->type = ext_rt->type_handlers[type_id];  /* Get type from handlers array */
    obj->ref_count = 1;  /* Start with reference count 1 */
    obj->gc_color = XC_GC_WHITE;
    
    /* Add to white list */
    obj->gc_next = gc->white_list;
    gc->white_list = obj;
    
    /* Update statistics */
    gc->used_memory += size;
    gc->total_allocated++;
    gc->allocation_count++;
    
    /* Expand heap size if needed */
    if (gc->used_memory > gc->heap_size) {
        gc->heap_size = (size_t)(gc->used_memory * gc->config.growth_factor);
        if (gc->heap_size > gc->config.max_heap_size) {
            gc->heap_size = gc->config.max_heap_size;
        }
    }
    
    return obj;
}

/* Free an object immediately */
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj) {
    xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    if (!obj) return;
    
    /* Call type-specific free function if available */
    if (obj->type && obj->type->free) {
        obj->type->free(rt, obj);
    }
    
    /* Remove from white list */
    xc_object_t *prev = NULL;
    xc_object_t *curr = gc->white_list;
    
    while (curr) {
        if (curr == obj) {
            if (prev) {
                prev->gc_next = curr->gc_next;
            } else {
                gc->white_list = curr->gc_next;
            }
            break;
        }
        prev = curr;
        curr = curr->gc_next;
    }
    
    /* Update statistics */
    gc->used_memory -= obj->size;
    gc->total_freed++;
    
    /* Free the memory */
    free(obj);
}

/* Mark an object as permanent (never collected) */
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    obj->gc_color = XC_GC_PERMANENT;
}

/* Add a reference to an object */
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    obj->ref_count++;
}

/* Release a reference to an object */
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    
    obj->ref_count--;
    
    /* If ref count is 0, free the object immediately */
    if (obj->ref_count <= 0) {
        xc_gc_free(rt, obj);
    }
}

/* Get the reference count of an object */
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return 0;
    return obj->ref_count;
}

/* Add a root pointer to the root set */
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr) {
    if (!root_ptr) return;
    
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Check if we need to expand the roots array */
    if (gc->root_count >= gc->root_capacity) {
        size_t new_capacity = gc->root_capacity == 0 ? 16 : gc->root_capacity * 2;
        xc_object_t ***new_roots = (xc_object_t ***)realloc(gc->roots, new_capacity * sizeof(xc_object_t **));
        if (!new_roots) {
            fprintf(stderr, "Failed to allocate roots array\n");
            return;
        }
        gc->roots = new_roots;
        gc->root_capacity = new_capacity;
    }
    
    /* Add root pointer to roots array */
    gc->roots[gc->root_count++] = root_ptr;
}

/* Remove a root pointer from the root set */
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr) {
    if (!root_ptr) return;
    
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Find the root pointer in the array */
    for (size_t i = 0; i < gc->root_count; i++) {
        if (gc->roots[i] == root_ptr) {
            /* Move the last element to this position */
            gc->roots[i] = gc->roots[--gc->root_count];
            return;
        }
    }
}

/* Get GC statistics */
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    xc_gc_stats_t stats;
    
    stats.heap_size = gc->heap_size;
    stats.used_memory = gc->used_memory;
    stats.total_allocated = gc->total_allocated;
    stats.total_freed = gc->total_freed;
    stats.gc_cycles = gc->gc_cycles;
    stats.avg_pause_time_ms = gc->gc_cycles > 0 ? gc->total_pause_time_ms / gc->gc_cycles : 0;
    stats.last_pause_time_ms = gc->gc_cycles > 0 ? 0 : 0; /* Not tracking last pause time */
    
    return stats;
}

/* Print GC statistics */
void xc_gc_print_stats(xc_runtime_t *rt) {
    xc_gc_stats_t stats = xc_gc_get_stats(rt);
    
    printf("XC Garbage Collector Statistics:\n");
    printf("  Heap Size: %zu bytes\n", stats.heap_size);
    printf("  Used Memory: %zu bytes\n", stats.used_memory);
    printf("  Memory Usage: %.2f%%\n", (double)stats.used_memory / stats.heap_size * 100.0);
    printf("  Total Allocated: %zu objects\n", stats.total_allocated);
    printf("  Total Freed: %zu objects\n", stats.total_freed);
    printf("  GC Cycles: %zu\n", stats.gc_cycles);
    printf("  Average Pause Time: %.2f ms\n", stats.avg_pause_time_ms);
}

/* Enable garbage collection */
void xc_gc_enable(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    gc->enabled = true;
}

/* Disable garbage collection */
void xc_gc_disable(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    gc->enabled = false;
}

/* Check if garbage collection is enabled */
bool xc_gc_is_enabled(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    return gc->enabled;
}

/**
 * Public API for garbage collection
 */

/**
 * Collect garbage
 */
void xc_gc_collect(void) {
    xc_runtime_t *rt = &xc;
    rt->gc();
}

/**
 * Release an object
 */
void xc_gc_release_object(xc_val obj) {
    xc_runtime_t *rt = &xc;
    xc_gc_release(rt, (xc_object_t *)obj);
}

/**
 * Global release function for backward compatibility
 */
void xc_release(xc_val obj) {
    xc_runtime_t *rt = &xc;
    xc_gc_release(rt, (xc_object_t *)obj);
}
