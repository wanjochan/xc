#include "xc.h"
#include "xc_internal.h"
/*
notes
缺少的机制：
弱引用：没有发现弱引用相关的实现
终结器：没有发现对象终结器（finalizer）的实现
显式解除引用：没有专门的 API 来解除对象引用关系
*/
/* Garbage collector color marks for tri-color marking */
#define XC_GC_WHITE      0   /* Object is not reachable (candidate for collection) */
#define XC_GC_GRAY       1   /* Object is reachable but its children haven't been scanned */
#define XC_GC_BLACK      2   /* Object is reachable and its children have been scanned */
#define XC_GC_PERMANENT  3   /* Object is permanently reachable (never collected) */

/* 执行垃圾回收 */
void xc_gc(void) {
    xc_runtime_t *rt = &xc;
    xc_gc_run(rt);
}

/* Get GC context from runtime */
static xc_gc_context_t *xc_gc_get_context(xc_runtime_t *rt) {
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    return gc;
}

/* Initialize the garbage collector for thread-local context */
void xc_gc_init_auto(xc_runtime_t *rt, const xc_gc_config_t *config) {
    // 使用全局定义的线程本地变量
    if (xc_gc_context) {
        // GC already initialized for this thread
        return;
    }
    
    // 创建 GC 上下文
    xc_gc_context = (xc_gc_context_t *)malloc(sizeof(xc_gc_context_t));
    if (!xc_gc_context) {
        fprintf(stderr, "Failed to allocate GC context for thread\n");
        return;
    }
    
    // 初始化 GC 上下文
    memset(xc_gc_context, 0, sizeof(xc_gc_context_t));
    
    // 设置配置
    if (config) {
        xc_gc_context->config = *config;
    } else {
        // 默认配置
        xc_gc_context->config.initial_heap_size = 1024 * 1024; // 1MB
        xc_gc_context->config.max_heap_size = 1024 * 1024 * 1024; // 1GB
        xc_gc_context->config.growth_factor = 1.5;
        xc_gc_context->config.gc_threshold = 0.75;
        xc_gc_context->config.max_alloc_before_gc = 10000;
    }
    
    // 初始化堆
    xc_gc_context->heap_size = xc_gc_context->config.initial_heap_size;
    xc_gc_context->used_memory = 0;
    xc_gc_context->allocation_count = 0;
    xc_gc_context->gc_cycles = 0;
    xc_gc_context->total_allocated = 0;
    xc_gc_context->total_freed = 0;
    xc_gc_context->enabled = true;
    
    // 初始化对象列表
    xc_gc_context->white_list = NULL;
    xc_gc_context->gray_list = NULL;
    xc_gc_context->black_list = NULL;
    xc_gc_context->roots = NULL;
    xc_gc_context->root_count = 0;
    xc_gc_context->root_capacity = 0;
    
    // 设置到运行时 - 这里不需要设置，因为 xc_gc_context 是全局变量
    
    printf("DEBUG: GC initialized for thread, context=%p\n", xc_gc_context);
}

/* Shutdown the garbage collector */
void xc_gc_shutdown(xc_runtime_t *rt) {
    // 使用全局变量
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    if (!gc) {
        return;
    }
    
    // 释放根集合
    if (gc->roots) {
        free(gc->roots);
        gc->roots = NULL;
    }
    
    // 释放 GC 上下文
    free(gc);
    xc_gc_context = NULL;
}

/* Mark phase of GC - traverse object and mark all reachable objects */
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj || obj->gc_color != XC_GC_WHITE) {
        return;  // 已经标记过或不需要标记
    }
    
    // 将对象标记为灰色
    obj->gc_color = XC_GC_GRAY;
    
    // 获取GC上下文
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    if (!gc) {
        return;
    }
    
    // 将对象添加到灰色列表
    obj->gc_next = gc->gray_list;
    gc->gray_list = obj;
}

/* 标记值为可达（用于 marker 函数） */
void xc_gc_mark_val(xc_val val) {
    // 获取当前运行时
    xc_runtime_t *rt = &xc;
    
    // 转换为对象指针并标记
    xc_object_t *obj = (xc_object_t *)val;
    xc_gc_mark(rt, obj);
}

/* Process gray list and mark all reachable objects */
static void xc_gc_process_gray_list(xc_runtime_t *rt) {
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    
    // 处理灰色对象列表
    xc_object_t *obj = gc->gray_list;
    gc->gray_list = NULL;
    
    while (obj) {
        // 将对象标记为黑色
        obj->gc_color = XC_GC_BLACK;
        
        // 将对象从灰色列表移到黑色列表
        xc_object_t *next = obj->gc_next;
        obj->gc_next = gc->black_list;
        gc->black_list = obj;
        
        // 标记对象引用的其他对象
        xc_type_lifecycle_t *type_handler = get_type_handler(obj->type_id);
        if (type_handler && type_handler->marker) {
            type_handler->marker((xc_val)obj, xc_gc_mark_val);
        }
        
        obj = next;
    }
}

/* Sweep phase of GC - detect and free unreachable objects */
static size_t xc_gc_sweep(xc_runtime_t *rt) {
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    size_t freed_count = 0;
    
    // 遍历白色对象列表，释放未标记的对象
    xc_object_t *curr = gc->white_list;
    xc_object_t *prev = NULL;
    
    while (curr) {
        xc_object_t *next = curr->gc_next;
        
        // 如果对象是白色的，释放它
        if (curr->gc_color == XC_GC_WHITE) {
            // 调用类型特定的释放函数
            xc_type_lifecycle_t *type_handler = get_type_handler(curr->type_id);
            if (type_handler && type_handler->destroyer) {
                type_handler->destroyer((xc_val)curr);
            }
            
            // 从链表中移除
            if (prev) {
                prev->gc_next = next;
            } else {
                gc->white_list = next;
            }
            
            // 释放内存
            free(curr);
            freed_count++;
        } else {
            // 保留对象
            prev = curr;
        }
        
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
}

/* Reset object colors for next GC cycle */
static void xc_gc_reset_colors(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Move all black objects to white list */
    while (gc->black_list) {
        xc_object_t *obj = gc->black_list;
        gc->black_list = obj->gc_next;
        
        /* Skip permanent objects */
        if (obj->gc_color == XC_GC_PERMANENT) {
            continue;
        }
        
        /* Reset color to white */
        obj->gc_color = XC_GC_WHITE;
        
        /* Add to white list */
        obj->gc_next = gc->white_list;
        gc->white_list = obj;
    }
}

void xc_gc_run(xc_runtime_t *rt) {
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Skip if GC is disabled */
    if (!gc->enabled) return;
    
    /* Record start time */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Reset object colors */
    xc_gc_reset_colors(rt);
    
    /* Mark phase */
    xc_gc_mark_roots(rt);
    xc_gc_process_gray_list(rt);
    
    /* Sweep phase */
    size_t freed = xc_gc_sweep(rt);
    
    /* Record end time and calculate pause time */
    clock_gettime(CLOCK_MONOTONIC, &end);
    double pause_time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                          (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    /* Update statistics */
    gc->gc_cycles++;
    gc->total_pause_time_ms += pause_time_ms;
    gc->allocation_count = 0;
    
    /* Print debug info if needed */
    #ifdef XC_DEBUG_GC
    printf("GC: freed %zu objects, pause time %.2f ms\n", freed, pause_time_ms);
    #endif
}

/* Allocate a new object */
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id) {
    // 使用全局变量
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    if (!gc) {
        fprintf(stderr, "GC not initialized\n");
        return NULL;
    }
    
    // 检查是否需要运行 GC
    gc->allocation_count++;
    if (gc->enabled && 
        (gc->allocation_count >= gc->config.max_alloc_before_gc ||
         gc->used_memory >= gc->heap_size * gc->config.gc_threshold)) {
        xc_gc_run(rt);
    }
    
    // 分配内存 - 确保使用新的内存地址
    void *memory = malloc(size);
    if (!memory) {
        fprintf(stderr, "Failed to allocate object of size %zu\n", size);
        return NULL;
    }
    
    // 转换为对象指针
    xc_object_t *obj = (xc_object_t *)memory;
    
    printf("DEBUG: xc_gc_alloc 分配内存 %p，大小 %zu，类型 %d\n", obj, size, type_id);
    
    // 初始化对象
    memset(obj, 0, size);
    obj->size = size;
    // obj->ref_count = 1;
    obj->gc_color = XC_GC_WHITE;
    
    // 设置类型ID
    obj->type_id = type_id;
    
    // 更新统计信息
    gc->used_memory += size;
    gc->total_allocated++;
    
    // 添加到白色列表
    obj->gc_next = gc->white_list;
    gc->white_list = obj;
    
    return obj;
}

/* Free an object */
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj) {
    printf("TODO deprecated xc_gc_free?\n");
    // if (!obj) {
    //     return;
    // }
    
    // // 减少引用计数
    // obj->ref_count--;
    
    // // 如果引用计数为0，释放对象
    // if (obj->ref_count <= 0) {
    //     // 调用类型特定的释放函数
    //     xc_type_lifecycle_t *type_handler = get_type_handler(obj->type_id);
    //     if (type_handler && type_handler->destroyer) {
    //         type_handler->destroyer((xc_val)obj);
    //     }
        
    //     // 释放内存
    //     free(obj);
    // }
}

/* Mark an object as permanently reachable */
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    obj->gc_color = XC_GC_BLACK;
}

/* Add a reference to an object */
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj) {
    printf("TODO deprecated xc_gc_add_ref?\n");
    // if (!obj) return;
    // obj->ref_count++;
}

/* Release a reference to an object */
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj) {
    printf("TODO deprecated xc_gc_release?\n");
    // if (!obj) return;
    
    // obj->ref_count--;
    
    // /* If reference count reaches zero, free the object */
    // if (obj->ref_count <= 0) {
    //     xc_gc_free(rt, obj);
    // }
}

// /* Get the reference count of an object */
// int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj) {
//     printf("TODO deprecated xc_gc_get_ref_count?\n");
//     // if (!obj) return 0;
//     // return obj->ref_count;
// }

/* Add a root object to the root set */
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr) {
    if (!root_ptr) return;
    
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Check if we need to resize the roots array */
    if (gc->root_count >= gc->root_capacity) {
        size_t new_capacity = gc->root_capacity == 0 ? 16 : gc->root_capacity * 2;
        xc_object_t ***new_roots = (xc_object_t ***)realloc(gc->roots, new_capacity * sizeof(xc_object_t **));
        if (!new_roots) {
            fprintf(stderr, "Failed to resize roots array\n");
            return;
        }
        gc->roots = new_roots;
        gc->root_capacity = new_capacity;
    }
    
    /* Add root to the array */
    gc->roots[gc->root_count++] = root_ptr;
}

/* Remove a root object from the root set */
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr) {
    if (!root_ptr) return;
    
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Find and remove the root */
    for (size_t i = 0; i < gc->root_count; i++) {
        if (gc->roots[i] == root_ptr) {
            /* Move the last root to this position */
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
    stats.last_pause_time_ms = 0;  /* Not tracked currently */
    
    return stats;
}

/* Print GC statistics */
void xc_gc_print_stats(xc_runtime_t *rt) {
    xc_gc_stats_t stats = xc_gc_get_stats(rt);
    
    printf("GC Statistics:\n");
    printf("  Heap size: %zu bytes\n", stats.heap_size);
    printf("  Used memory: %zu bytes (%.2f%%)\n", 
           stats.used_memory, 
           stats.heap_size > 0 ? (double)stats.used_memory / stats.heap_size * 100 : 0);
    printf("  Total allocated: %zu objects\n", stats.total_allocated);
    printf("  Total freed: %zu objects\n", stats.total_freed);
    printf("  GC cycles: %zu\n", stats.gc_cycles);
    printf("  Average pause time: %.2f ms\n", stats.avg_pause_time_ms);
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

// /* Global release function for backward compatibility */
// void xc_gc_release_object(xc_val obj) {
//     /* This is a placeholder for backward compatibility */
//     /* In a real implementation, we would need to get the current runtime */
//     /* and call xc_gc_release on it */
// }

// /* Global release function for backward compatibility */
// void xc_release(xc_val obj) {
//     printf("TODO remove deprecated xc_release?\n");
//     // xc_gc_release_object(obj);
// }

// /* 分配原始内存并处理GC相关逻辑 */
// void* xc_gc_allocate_raw_memory(size_t size, int type_id) {
//     xc_gc_init_auto(&xc, NULL);//TODO config from global one...
//     // 分配内存
//     void* memory = malloc(size);
//     if (!memory) return NULL;
    
//     // 初始化 GC 相关字段
//     xc_header_t* header = (xc_header_t*)memory;
//     header->size = size;
//     header->type = type_id;  // 使用type而不是type_id以匹配原有代码
//     header->flags = 0;
//     header->color = XC_GC_WHITE;
//     header->ref_count = 1;  // 初始引用计数为1

//     return memory;
// } 
