#include "xc.h"
#include "xc_internal.h"

void xc_gc_thread_init_auto(void) {
    // /*
    // if (_thread_gc.initialized) {
    //     return;
    // }
    // /* 初始化垃圾回收器状态 */
    // _thread_gc.gc_first = NULL;
    // _thread_gc.total_memory = 0;
    // _thread_gc.gc_threshold = 1024 * 1024; /* 1MB */
    // _thread_gc.initialized = 1;
    
    // /* 初始化灰色对象栈 */
    // _thread_gc.gray_count = 0;
    // _thread_gc.gray_capacity = 256;
    // _thread_gc.gray_stack = (xc_header_t**)malloc(_thread_gc.gray_capacity * sizeof(xc_header_t*));
    
    // /* 初始化其他字段 */
    // _thread_gc.allocation_count = 0;
    
    // /* 记录当前栈底位置 */
    // int dummy;
    // _thread_gc.stack_bottom = &dummy;
    // */
    
    /* 初始化线程状态 */
    _xc_thread_state.top = NULL;
    _xc_thread_state.depth = 0;
    _xc_thread_state.current = NULL;
    _xc_thread_state.current_error = NULL;
    _xc_thread_state.in_try_block = false;
    _xc_thread_state.uncaught_handler = NULL;

    atexit(xc_gc_thread_exit);
}

void xc_gc_thread_exit(void) {
    // /*
    // if (_thread_gc.initialized) {
    //     /* 手动释放所有剩余对象，但不调用终结器 */
    //     xc_header_t* current = _thread_gc.gc_first;
    //     while (current) {
    //         xc_header_t* next = current->next_gc;
    //         free(current);
    //         current = next;
    //     }
        
    //     /* 释放灰色栈 */
    //     if (_thread_gc.gray_stack) {
    //         free(_thread_gc.gray_stack);
    //         _thread_gc.gray_stack = NULL;
    //     }
        
    //     _thread_gc.gc_first = NULL;
    //     _thread_gc.initialized = 0;
    // }
    // */
}

/* 将对象标记为灰色并加入灰色栈 */
// /*
// static void gc_mark_gray(xc_header_t* header) {
//     if (!header || header->color != XC_GC_WHITE) {
//         return;
//     }
    
//     header->color = XC_GC_GRAY;
    
//     /* 确保灰色栈有足够空间 */
//     if (_thread_gc.gray_count >= _thread_gc.gray_capacity) {
//         _thread_gc.gray_capacity *= 2;
//         _thread_gc.gray_stack = realloc(_thread_gc.gray_stack, 
//             sizeof(xc_header_t*) * _thread_gc.gray_capacity);
//     }
    
//     _thread_gc.gray_stack[_thread_gc.gray_count++] = header;
// }
// */

/* 扫描灰色对象 */
// /*
// static void gc_scan_gray(void) {
//     while (_thread_gc.gray_count > 0) {
//         xc_header_t* header = _thread_gc.gray_stack[--_thread_gc.gray_count];
//         if (!header) continue;
        
//         /* 获取类型特定的标记函数 */
//         int type = header->type;
//         /* 确保类型ID在有效范围内 */
//         if (type < 0 || type >= TYPE_HASH_SIZE) continue;
        
//         xc_type_entry_t* entry = type_registry.buckets[type];
//         if (entry && entry->lifecycle.marker) {
//             entry->lifecycle.marker(XC_OBJECT(header), gc_mark_object);
//         }
        
//         /* 标记为黑色 */
//         header->color = XC_GC_BLACK;
//     }
// }
// */

/* 标记对象 */
/*
static void gc_mark_object(xc_val obj) {
    if (!obj) return;
    
    xc_header_t* header = XC_HEADER(obj);
    gc_mark_gray(header);
}
*/

/* 标记栈上的对象 */
// /*
// static void gc_mark_stack(void) {
//     jmp_buf env;
//     setjmp(env); /* 刷新寄存器到栈上 */
    
//     void* stack_top = &env;
//     void* stack_bottom = _thread_gc.stack_bottom;
    
//     /* 确保栈底指针有效 */
//     if (!stack_bottom) {
//         return;
//     }
    
//     /* 计算栈的范围 */
//     void* scan_start = stack_top;
//     void* scan_end = stack_bottom;
    
//     /* 确保扫描方向正确 */
//     if (scan_start > scan_end) {
//         void* temp = scan_start;
//         scan_start = scan_end;
//         scan_end = temp;
//     }
    
//     /* 扫描栈空间，寻找可能的对象引用 */
//     for (void* p = scan_start; p <= scan_end; p = (char*)p + sizeof(void*)) {
//         void* ptr = *(void**)p;
        
//         /* 检查指针是否可能是有效的对象引用 */
//         if (ptr) {
//             /* 检查指针是否在合理的内存范围内 */
//             uintptr_t ptr_val = (uintptr_t)ptr;
//             if (ptr_val % sizeof(void*) == 0) {  /* 指针应该是对齐的 */
//                 gc_mark_object(ptr);
//             }
//         }
//     }
// }
// */

/* 标记根对象 */
/*
static void gc_mark_roots(void) {
    xc_header_t* current = _thread_gc.gc_first;
    while (current) {
        if (current->flags & XC_FLAG_ROOT) {
            gc_mark_gray(current);
        }
        current = current->next_gc;
    }
}
*/

/* 清除未标记对象 */
// /*
// static void gc_sweep(void) {
//     xc_header_t* current = _thread_gc.gc_first;
//     xc_header_t* prev = NULL;
    
//     while (current) {
//         xc_header_t* next = current->next_gc;
        
//         if (current->color == XC_GC_WHITE) {
//             /* 对象未被标记，需要回收 */
//             if (prev) {
//                 prev->next_gc = next;
//             } else {
//                 _thread_gc.gc_first = next;
//             }
            
//             /* 调用终结器 */
//             if (current->flags & XC_FLAG_FINALIZE) {
//                 xc_type_entry_t* entry = type_registry.buckets[current->type];
//                 if (entry && entry->lifecycle.destroyer) {
//                     entry->lifecycle.destroyer(XC_OBJECT(current));
//                 }
//             }
            
//             _thread_gc.total_memory -= current->size;
//             free(current);
//             current = next;
//         } else {
//             /* 重置对象颜色为白色，为下次GC做准备 */
//             current->color = XC_GC_WHITE;
//             prev = current;
//             current = next;
//         }
//     }
    
//     /* 调整GC阈值 */
//     if (_thread_gc.total_memory > 0) {
//         _thread_gc.gc_threshold = _thread_gc.total_memory * 2;
//     } else {
//         _thread_gc.gc_threshold = 1024 * 1024; /* 1MB */
//     }
// }
// */

/* 执行垃圾回收 */
void xc_gc(void) {
    // /*
    // xc_gc_thread_init_auto();
    
    // /* 确保已初始化 */
    // if (!_thread_gc.initialized) {
    //     return;
    // }
    
    // /* 将所有对象标记为白色 */
    // xc_header_t* current = _thread_gc.gc_first;
    // while (current) {
    //     current->color = XC_GC_WHITE;
    //     current = current->next_gc;
    // }
    
    // /* 标记阶段 */
    // // gc_mark_roots();    /* 标记根对象 */
    // // gc_mark_stack();    /* 标记栈上对象 */
    // // gc_scan_gray();     /* 处理灰色对象 */
    
    // /* 清除阶段 */
    // // gc_sweep();
    
    // /* 调整GC阈值 */
    // if (_thread_gc.total_memory > 0) {
    //     _thread_gc.gc_threshold = _thread_gc.total_memory * 2;
    // } else {
    //     _thread_gc.gc_threshold = 1024 * 1024; /* 1MB */
    // }
    
    // /* 重置分配计数 */
    // _thread_gc.allocation_count = 0;
    // */
    
    // 使用 xc_gc_context 的 GC 而不是 _thread_gc
    xc_runtime_t *rt = &xc;
    xc_gc_run(rt);
}

/* Get GC context from runtime */
static xc_gc_context_t *xc_gc_get_context(xc_runtime_t *rt) {
    xc_gc_context_t *gc = (xc_gc_context_t *)xc_gc_context;
    return gc;
}

/* Initialize the garbage collector */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config) {
    // 使用全局变量
    if (xc_gc_context) {
        // GC already initialized
        return;
    }
    
    // 创建 GC 上下文
    xc_gc_context_t *gc = (xc_gc_context_t *)malloc(sizeof(xc_gc_context_t));
    if (!gc) {
        fprintf(stderr, "Failed to allocate GC context\n");
        return;
    }
    
    // 初始化 GC 上下文
    memset(gc, 0, sizeof(xc_gc_context_t));
    
    // 设置配置
    if (config) {
        gc->config = *config;
    } else {
        // 默认配置
        gc->config.initial_heap_size = 1024 * 1024; // 1MB
        gc->config.max_heap_size = 1024 * 1024 * 1024; // 1GB
        gc->config.growth_factor = 1.5;
        gc->config.gc_threshold = 0.75;
        gc->config.max_alloc_before_gc = 10000;
    }
    
    // 初始化根集合
    gc->roots = NULL;
    gc->root_count = 0;
    gc->root_capacity = 0;
    
    // 初始化对象列表
    gc->white_list = NULL;
    gc->gray_list = NULL;
    gc->black_list = NULL;
    
    // 启用 GC
    gc->enabled = true;
    
    // 保存 GC 上下文
    xc_gc_context = gc;
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


/* Run a garbage collection cycle */
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
    
    // 分配内存
    xc_object_t *obj = (xc_object_t *)malloc(size);
    if (!obj) {
        fprintf(stderr, "Failed to allocate object of size %zu\n", size);
        return NULL;
    }
    
    // 初始化对象
    memset(obj, 0, size);
    obj->size = size;
    obj->ref_count = 1;
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
    if (!obj) {
        return;
    }
    
    // 减少引用计数
    obj->ref_count--;
    
    // 如果引用计数为0，释放对象
    if (obj->ref_count <= 0) {
        // 调用类型特定的释放函数
        xc_type_lifecycle_t *type_handler = get_type_handler(obj->type_id);
        if (type_handler && type_handler->destroyer) {
            type_handler->destroyer((xc_val)obj);
        }
        
        // 释放内存
        free(obj);
    }
}

/* Mark an object as permanently reachable */
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    obj->gc_color = XC_GC_BLACK;
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
    
    /* If reference count reaches zero, free the object */
    if (obj->ref_count <= 0) {
        xc_gc_free(rt, obj);
    }
}

/* Get the reference count of an object */
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return 0;
    return obj->ref_count;
}

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

/* Global release function for backward compatibility */
void xc_gc_release_object(xc_val obj) {
    /* This is a placeholder for backward compatibility */
    /* In a real implementation, we would need to get the current runtime */
    /* and call xc_gc_release on it */
}

/* Global release function for backward compatibility */
void xc_release(xc_val obj) {
    xc_gc_release_object(obj);
}

/* 分配原始内存并处理GC相关逻辑 */
//TOD 稍后清除....
void* xc_gc_allocate_raw_memory(size_t size, int type_id) {
    // xc_gc_thread_init_auto();
    
    // 分配内存
    void* memory = malloc(size);
    if (!memory) return NULL;
    
    // 初始化 GC 相关字段
    xc_header_t* header = (xc_header_t*)memory;
    header->size = size;
    header->type = type_id;  // 使用type而不是type_id以匹配原有代码
    header->flags = 0;
    header->color = XC_GC_WHITE;
    header->ref_count = 1;  // 初始引用计数为1
    
    /*
    // 更新 GC 链表
    header->next_gc = _thread_gc.gc_first;
    _thread_gc.gc_first = header;
    _thread_gc.total_memory += header->size;
    
    // 增加分配计数
    _thread_gc.allocation_count++;
    
    // 检查是否需要进行垃圾回收
    if (_thread_gc.allocation_count > 1000 || _thread_gc.total_memory > _thread_gc.gc_threshold) {
        xc_gc();
    }
    */
    
    // 使用 xc_gc_context 而不是 _thread_gc
    // xc_runtime_t *rt = &xc;
    // xc_gc_add_object(rt, (xc_object_t*)memory);
    
    return memory;
} 

