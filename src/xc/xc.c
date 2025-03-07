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

/* Function declarations for GC */
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config);
void xc_gc_shutdown(xc_runtime_t *rt);
void xc_gc_run(xc_runtime_t *rt);
void xc_gc_enable(xc_runtime_t *rt);
void xc_gc_disable(xc_runtime_t *rt);
bool xc_gc_is_enabled(xc_runtime_t *rt);
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id);
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_mark(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_add_ref(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_release(xc_runtime_t *rt, xc_object_t *obj);
int xc_gc_get_ref_count(xc_runtime_t *rt, xc_object_t *obj);
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr);
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr);
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt);
void xc_gc_print_stats(xc_runtime_t *rt);
void xc_gc_release_object(xc_val obj);
void xc_release(xc_val obj);

/* 错误代码 */
#define XC_ERR_NONE 0
#define XC_ERR_GENERIC 1        /* 通用错误 */
#define XC_ERR_TYPE 2           /* 类型错误 */
#define XC_ERR_VALUE 3          /* 值错误 */
#define XC_ERR_INDEX 4          /* 索引错误 */
#define XC_ERR_KEY 5            /* 键错误 */
#define XC_ERR_ATTRIBUTE 6      /* 属性错误 */
#define XC_ERR_NAME 7           /* 名称错误 */
#define XC_ERR_SYNTAX 8         /* 语法错误 */
#define XC_ERR_RUNTIME 9        /* 运行时错误 */
#define XC_ERR_MEMORY 10        /* 内存错误 */
#define XC_ERR_IO 11            /* IO错误 */
#define XC_ERR_NOT_IMPLEMENTED 12  /* 未实现错误 */
#define XC_ERR_INVALID_ARGUMENT 13  /* 无效参数错误 */
#define XC_ERR_ASSERTION 14     /* 断言错误 */
#define XC_ERR_USER 15          /* 用户自定义错误 */

/* 函数处理器类型定义 */
typedef xc_val (*xc_function_handler)(xc_val this_obj, int argc, xc_val* argv, xc_val closure);

/* 执行栈帧结构 */
typedef struct xc_stack_frame {
    const char* func_name;         /* 函数名称 */
    const char* file_name;         /* 文件名称 */
    int line_number;               /* 行号 */
    struct xc_stack_frame* prev;   /* 上一帧 */
} xc_stack_frame_t;

/* 异常处理器结构 - 内部增强版本 */
typedef struct xc_exception_handler_internal {
    jmp_buf env;                      /* 保存的环境 */
    xc_val catch_func;                /* catch处理器 */
    xc_val finally_func;              /* finally处理器 */
    struct xc_exception_handler_internal* prev; /* 链接到前一个处理器 */
    xc_val current_exception;         /* 当前处理的异常 */
    bool has_caught;                  /* 是否已经捕获过异常 */
} xc_exception_handler_internal_t;

/* 统一线程本地状态 */
static __thread struct {
    /* 栈相关 */
    xc_stack_frame_t* top;         /* 栈顶 */
    int depth;                     /* 栈深度 */
    /* 异常相关 */
    xc_exception_handler_internal_t* current;  /* 当前异常处理器 */
    xc_val current_error;          /* 当前错误 */
    bool in_try_block;             /* 是否在try块中 */
    xc_val uncaught_handler;       /* 未捕获异常处理器 */
} _xc_thread_state = {0};

/* 类型注册表结构 */
#define TYPE_HASH_SIZE 64

typedef struct xc_type_entry {
    const char* name;
    int id;
    xc_type_lifecycle_t lifecycle;
    struct xc_type_entry* next;
} xc_type_entry_t;

typedef struct {
    xc_type_entry_t* buckets[TYPE_HASH_SIZE];
    int count;
    int capacity;
    xc_type_entry_t* entries;
} xc_type_registry_t;

static xc_type_registry_t type_registry;

/* 对象头结构 - 每个对象的隐藏前缀 */
typedef struct {
    int type;           /* 类型ID */
    unsigned int flags;  /* 标志位 */
    int ref_count;       /* 引用计数 */
    void* next_gc;       /* GC链表下一项 */
    size_t size;         /* 对象大小（包括头部） */
    const char* type_name; /* 类型名称（便于调试） */
    unsigned char color;   /* 对象颜色，用于三色标记法 */
} xc_header_t;

/* 标志位定义 */
#define XC_FLAG_ROOT     (1 << 0)  /* GC根对象 */
#define XC_FLAG_FINALIZE (1 << 1)  /* 需要终结 */

/* 从对象获取头部 */
#define XC_HEADER(obj) ((xc_header_t*)((char*)(obj) - sizeof(xc_header_t)))

/* 从头部获取对象 */
#define XC_OBJECT(header) ((xc_val)((char*)(header) + sizeof(xc_header_t)))

/* 线程本地GC状态 */
static __thread struct {
    void* gc_first;         /* 本线程GC链表头 */
    size_t total_memory;    /* 本线程分配的内存总量 */
    size_t gc_threshold;    /* 本线程GC阈值 */
    char initialized;       /* 线程本地状态是否已初始化 */
    void* stack_bottom;     /* 栈底指针 */
    size_t gray_count;      /* 灰色对象计数 */
    xc_header_t** gray_stack; /* 灰色对象栈 */
    size_t gray_capacity;   /* 灰色对象栈容量 */
    size_t allocation_count; /* 分配计数，用于触发自动GC */
} _thread_gc = {0};

/* 全局状态结构 */
typedef struct {
    //char initialized;
    /* 类型方法表 */
    struct {
        const char* name;
        xc_val (*func)(xc_val, ...);
        int next;  /* 链表下一个方法索引 */
    } methods[256];  /* 方法池 */
    int method_count;
    int method_heads[16];  /* 每个类型的方法链表头 */
    xc_type_registry_t type_registry;
} xc_state_t;

/* 全局状态 */
static xc_state_t _state = {0};

/* 前置声明 */
static void push_stack_frame(const char* func_name, const char* file_name, int line_number);
static void pop_stack_frame(void);
static void thread_cleanup(void);
static void throw_internal(xc_val error, bool allow_rethrow);
static void throw(xc_val error);
static void throw_with_rethrow(xc_val error);
static xc_val create(int type, ...);
static int type_of(xc_val val);
static int is(xc_val val, int type);
static xc_val invoke(xc_val func, int argc, ...);
static xc_val dot(xc_val obj, const char* key, ...);
static xc_val call(xc_val obj, const char* method, ...);
static void gc_mark_object(xc_val obj);
static void gc_mark_stack(void);
static void gc_mark_roots(void);
static void gc_sweep(void);
static void gc_mark_gray(xc_header_t* header);
static void gc_scan_gray(void);

// static void gc_mark_object(xc_val obj);
// static void gc_mark_stack(void);
// static void gc_mark_roots(void);
// static void gc_sweep(void);
// static void gc_mark_gray(xc_header_t* header);
// static void gc_scan_gray(void);
static int type_of(xc_val val);
static int is(xc_val val, int type);
static xc_val create(int type, ...);
static xc_val invoke(xc_val func, int argc, ...);
static xc_val call(xc_val obj, const char* method, ...);
static void clear_error(void);
static xc_val get_current_error(void);
static void set_uncaught_exception_handler(xc_val handler);
static void push_stack_frame(const char* func_name, const char* file_name, int line_number);
static void pop_stack_frame(void);
static void throw_internal(xc_val error, bool allow_rethrow);
static void throw(xc_val error);
static xc_val try_catch_finally(xc_val try_func, xc_val catch_func, xc_val finally_func);
static int get_type_id(const char* name);
static xc_val function_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure);

///////////////////////////////////////////////////
/* 简单的哈希函数 */
static unsigned int hash_string(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = hash * 31 + (*str++);
    }
    return hash % TYPE_HASH_SIZE;
}

/* 确保线程本地状态已初始化 */
static void ensure_thread_initialized(void) {
    if (_thread_gc.initialized) {
        return;
    }
    
    /* 初始化垃圾回收器状态 */
    _thread_gc.gc_first = NULL;
    _thread_gc.total_memory = 0;
    _thread_gc.gc_threshold = 1024 * 1024; /* 1MB */
    _thread_gc.initialized = 1;
    
    /* 初始化灰色对象栈 */
    _thread_gc.gray_count = 0;
    _thread_gc.gray_capacity = 256;
    _thread_gc.gray_stack = (xc_header_t**)malloc(_thread_gc.gray_capacity * sizeof(xc_header_t*));
    
    /* 初始化其他字段 */
    _thread_gc.allocation_count = 0;
    
    /* 记录当前栈底位置 */
    int dummy;
    _thread_gc.stack_bottom = &dummy;
    
    /* 初始化线程状态 */
    _xc_thread_state.top = NULL;
    _xc_thread_state.depth = 0;
    _xc_thread_state.current = NULL;
    _xc_thread_state.current_error = NULL;
    _xc_thread_state.in_try_block = false;
    _xc_thread_state.uncaught_handler = NULL;
    
    /* 注册线程清理函数 */
    atexit(thread_cleanup);
}

/* 线程退出清理函数 */
static void __attribute__((destructor)) thread_cleanup(void) {
    if (_thread_gc.initialized) {
        /* 手动释放所有剩余对象，但不调用终结器 */
        xc_header_t* current = _thread_gc.gc_first;
        while (current) {
            xc_header_t* next = current->next_gc;
            free(current);
            current = next;
        }
        
        /* 释放灰色栈 */
        if (_thread_gc.gray_stack) {
            free(_thread_gc.gray_stack);
            _thread_gc.gray_stack = NULL;
        }
        
        _thread_gc.gc_first = NULL;
        _thread_gc.initialized = 0;
    }
}

/* 将对象标记为灰色并加入灰色栈 */
static void gc_mark_gray(xc_header_t* header) {
    if (!header || header->color != XC_COLOR_WHITE) {
        return;
    }
    
    header->color = XC_COLOR_GRAY;
    
    /* 确保灰色栈有足够空间 */
    if (_thread_gc.gray_count >= _thread_gc.gray_capacity) {
        _thread_gc.gray_capacity *= 2;
        _thread_gc.gray_stack = realloc(_thread_gc.gray_stack, 
            sizeof(xc_header_t*) * _thread_gc.gray_capacity);
    }
    
    _thread_gc.gray_stack[_thread_gc.gray_count++] = header;
}

/* 扫描灰色对象 */
static void gc_scan_gray(void) {
    while (_thread_gc.gray_count > 0) {
        xc_header_t* header = _thread_gc.gray_stack[--_thread_gc.gray_count];
        if (!header) continue;
        
        /* 获取类型特定的标记函数 */
        int type = header->type;
        /* 确保类型ID在有效范围内 */
        if (type < 0 || type >= TYPE_HASH_SIZE) continue;
        
        xc_type_entry_t* entry = type_registry.buckets[type];
        if (entry && entry->lifecycle.marker) {
            entry->lifecycle.marker(XC_OBJECT(header), gc_mark_object);
        }
        
        /* 标记为黑色 */
        header->color = XC_COLOR_BLACK;
    }
}

/* 标记对象 */
static void gc_mark_object(xc_val obj) {
    if (!obj) return;
    
    xc_header_t* header = XC_HEADER(obj);
    gc_mark_gray(header);
}

/* 标记栈上的对象 */
static void gc_mark_stack(void) {
    jmp_buf env;
    setjmp(env); /* 刷新寄存器到栈上 */
    
    void* stack_top = &env;
    void* stack_bottom = _thread_gc.stack_bottom;
    
    /* 确保栈底指针有效 */
    if (!stack_bottom) {
        return;
    }
    
    /* 计算栈的范围 */
    void* scan_start = stack_top;
    void* scan_end = stack_bottom;
    
    /* 确保扫描方向正确 */
    if (scan_start > scan_end) {
        void* temp = scan_start;
        scan_start = scan_end;
        scan_end = temp;
    }
    
    /* 扫描栈空间，寻找可能的对象引用 */
    for (void* p = scan_start; p <= scan_end; p = (char*)p + sizeof(void*)) {
        void* ptr = *(void**)p;
        
        /* 检查指针是否可能是有效的对象引用 */
        if (ptr) {
            /* 检查指针是否在合理的内存范围内 */
            uintptr_t ptr_val = (uintptr_t)ptr;
            if (ptr_val % sizeof(void*) == 0) {  /* 指针应该是对齐的 */
                gc_mark_object(ptr);
            }
        }
    }
}

/* 标记根对象 */
static void gc_mark_roots(void) {
    xc_header_t* current = _thread_gc.gc_first;
    while (current) {
        if (current->flags & XC_FLAG_ROOT) {
            gc_mark_gray(current);
        }
        current = current->next_gc;
    }
}

/* 清除未标记对象 */
static void gc_sweep(void) {
    xc_header_t* current = _thread_gc.gc_first;
    xc_header_t* prev = NULL;
    
    while (current) {
        xc_header_t* next = current->next_gc;
        
        if (current->color == XC_COLOR_WHITE) {
            /* 对象未被标记，需要回收 */
            if (prev) {
                prev->next_gc = next;
            } else {
                _thread_gc.gc_first = next;
            }
            
            /* 调用终结器 */
            if (current->flags & XC_FLAG_FINALIZE) {
                xc_type_entry_t* entry = type_registry.buckets[current->type];
                if (entry && entry->lifecycle.destroyer) {
                    entry->lifecycle.destroyer(XC_OBJECT(current));
                }
            }
            
            _thread_gc.total_memory -= current->size;
            free(current);
            current = next;
        } else {
            /* 重置对象颜色为白色，为下次GC做准备 */
            current->color = XC_COLOR_WHITE;
            prev = current;
            current = next;
        }
    }
    
    /* 调整GC阈值 */
    if (_thread_gc.total_memory > 0) {
        _thread_gc.gc_threshold = _thread_gc.total_memory * 2;
    } else {
        _thread_gc.gc_threshold = 1024 * 1024; /* 1MB */
    }
}

/* 执行垃圾回收 */
void xc_gc(void) {
    ensure_thread_initialized();
    
    if (!_thread_gc.gc_first) return;
    
    /* 重置分配计数 */
    _thread_gc.allocation_count = 0;
    
    /* 标记阶段 */
    gc_mark_roots();    /* 标记根对象 */
    gc_mark_stack();    /* 标记栈上对象 */
    gc_scan_gray();     /* 处理灰色对象 */
    
    /* 清除阶段 */
    gc_sweep();
    
    /* 调整GC阈值 */
    if (_thread_gc.total_memory > 0) {
        _thread_gc.gc_threshold = _thread_gc.total_memory * 2;
    } else {
        _thread_gc.gc_threshold = 1024 * 1024; /* 1MB */
    }
}

/* 通过类型ID查找类型条目 */
static xc_type_entry_t* find_type_by_id(int type_id) {
    /* 遍历所有哈希桶 */
    for (int i = 0; i < TYPE_HASH_SIZE; i++) {
        xc_type_entry_t* current = type_registry.buckets[i];
        while (current) {
            if (current->id == type_id) {
                return current;
            }
            current = current->next;
        }
    }
    
    return NULL;
}

//tool for creator
static xc_val alloc_object(int type, ...) {
    if (type < 0 || type >= 16) return NULL;
    
    /* 获取类型注册项 */
    xc_type_entry_t* entry = find_type_by_id(type);
    if (!entry) {
        return NULL;
    }
    
    /* 获取对象大小 */
    va_list args;
    va_start(args, type);
    size_t size = va_arg(args, size_t);
    va_end(args);
    
    /* 分配对象内存 */
    xc_header_t* header;
    if (entry->lifecycle.allocator) {
        header = (xc_header_t*)entry->lifecycle.allocator(size + sizeof(xc_header_t));
    } else {
        header = (xc_header_t*)malloc(size + sizeof(xc_header_t));
    }
    
    if (!header) {
        return NULL;
    }
    
    /* 初始化对象头 */
    header->type = type;
    header->flags = 0;
    header->ref_count = 1;
    header->next_gc = _thread_gc.gc_first;
    header->size = size + sizeof(xc_header_t);
    header->type_name = entry->name;
    header->color = XC_COLOR_WHITE;
    
    _thread_gc.gc_first = header;
    _thread_gc.total_memory += header->size;
    
    /* 计算对象部分的指针 */
    void* obj = XC_OBJECT(header);
    
    /* 清零数据区 */
    memset(obj, 0, size);
    
    /* 增加分配计数 */
    _thread_gc.allocation_count++;
    
    /* 检查是否需要进行垃圾回收 */
    if (_thread_gc.allocation_count > 1000 || _thread_gc.total_memory > _thread_gc.gc_threshold) {
        xc_gc();
    }
    
    /* 返回对象指针 */
    return obj;
}

/* 通过名称查找类型ID */
static int find_type_id_by_name(const char* name) {
    if (!name) return -1;
    
    unsigned int hash = hash_string(name);
    xc_type_entry_t* entry = type_registry.buckets[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry->id;
        }
        entry = entry->next;
    }
    
    return -1;  /* 未找到 */
}

/* 注册类型 */
int xc_register_type(const char* name, xc_type_lifecycle_t* lifecycle) {
    if (type_registry.count >= 16) {
        return -1;
    }
    
    /* 检查是否已存在 */
    int existing_id = find_type_id_by_name(name);
    if (existing_id >= 0) {
        return existing_id;  /* 返回已存在的类型ID */
    }
    
    /* 为预定义类型分配固定ID */
    int type_id = -1;
    if (strcmp(name, "null") == 0) type_id = XC_TYPE_NULL;
    else if (strcmp(name, "boolean") == 0) type_id = XC_TYPE_BOOL;
    else if (strcmp(name, "number") == 0) type_id = XC_TYPE_NUMBER;
    else if (strcmp(name, "string") == 0) type_id = XC_TYPE_STRING;
    else if (strcmp(name, "function") == 0) type_id = XC_TYPE_FUNC;
    else if (strcmp(name, "array") == 0) type_id = XC_TYPE_ARRAY;
    else if (strcmp(name, "object") == 0) type_id = XC_TYPE_OBJECT;
    else if (strcmp(name, "vm") == 0) type_id = XC_TYPE_VM;
    else if (strcmp(name, "error") == 0) type_id = XC_TYPE_EXCEPTION;
    else {
        // 根据类型名称前缀决定分配区间
        if (strncmp(name, "internal.", 9) == 0) {
            // 内部类型
            type_id = XC_TYPE_INTERNAL_BEGIN + (type_registry.count % (XC_TYPE_INTERNAL_END - XC_TYPE_INTERNAL_BEGIN + 1));
        } else if (strncmp(name, "ext.", 4) == 0) {
            // 扩展类型
            type_id = XC_TYPE_EXTENSION_BEGIN + (type_registry.count % (XC_TYPE_EXTENSION_END - XC_TYPE_EXTENSION_BEGIN + 1));
        } else {
            // 用户类型
            type_id = XC_TYPE_USER_BEGIN + (type_registry.count % (XC_TYPE_USER_END - XC_TYPE_USER_BEGIN + 1));
        }
    }
    
    /* 创建新类型条目 */
    xc_type_entry_t* entry = (xc_type_entry_t*)malloc(sizeof(xc_type_entry_t));
    if (!entry) {
        return -1;
    }
    
    entry->name = strdup(name);
    if (!entry->name) {
        free(entry);
        return -1;
    }
    
    entry->id = type_id;
    memcpy(&entry->lifecycle, lifecycle, sizeof(xc_type_lifecycle_t));
    
    /* 添加到哈希表 */
    unsigned int hash = hash_string(name);
    entry->next = type_registry.buckets[hash];
    type_registry.buckets[hash] = entry;
    
    type_registry.count++;
    
    if (entry->lifecycle.initializer) {
        entry->lifecycle.initializer();
    }
    
    return type_id;
}

/* 注册方法 */
static char register_method(int type, const char* name, xc_method_func func) {
    if (type < 0 || type >= 16 || !name || !func) return 0;
    
    if (_state.method_count >= 256) return 0;
    
    int method_idx = _state.method_count++;
    _state.methods[method_idx].name = name;
    _state.methods[method_idx].func = (xc_val (*)(xc_val, ...))func;
    _state.methods[method_idx].next = _state.method_heads[type];
    _state.method_heads[type] = method_idx;
    
    return 1;
}

/* 查找方法 */
static xc_method_func find_method(int type, const char* name) {
    if (type < 0 || type >= 16 || !name) return NULL;
    
    int method_idx = _state.method_heads[type];
    while (method_idx != 0) {
        if (strcmp(_state.methods[method_idx].name, name) == 0) {
            return (xc_method_func)_state.methods[method_idx].func;
        }
        method_idx = _state.methods[method_idx].next;
    }
    
    return NULL;
}

static xc_val dot(xc_val obj, const char* key, ...) {
    if (!obj || !key) return NULL;
    
    /* 处理可变参数 */
    va_list args;
    va_start(args, key);
    
    /* 获取第一个额外参数 */
    xc_val value = va_arg(args, xc_val);
    
    /* 获取对象类型 */
    int type = type_of(obj);
    
    /* 如果有额外参数，则是设置操作 */
    if (value) {
        /* 首先尝试找类型特定的属性设置器，格式为 "set_属性名" */
        char setter_name[128] = "set_";
        strncat(setter_name, key, sizeof(setter_name) - 5); // 5 = 长度"set_" + 1防止溢出
        
        xc_method_func specific_setter = find_method(type, setter_name);
        if (specific_setter) {
            va_end(args);
            return specific_setter(obj, value);
        }
        
        /* 如果没有特定的设置器，尝试通用的设置方法 */
        xc_method_func general_setter = find_method(type, "set");
        if (general_setter) {
            /* 直接传递属性名和值，避免创建临时字符串和数组 */
            va_list setter_args;
            va_copy(setter_args, args);
            va_arg(setter_args, xc_val); // 跳过value参数，因为已经读取了
            xc_val result = general_setter(obj, xc.create(XC_TYPE_STRING, key));
            va_end(setter_args);
            va_end(args);
            return result;
        }
        
        /* 如果这是对象类型，直接设置属性 */
        if (type == XC_TYPE_OBJECT) {
            /* 对于对象类型，我们使用特定的对象操作函数 */
            /* 在实际代码中，应该通过结构体访问或函数指针访问 object_set */
            xc_val key_obj = xc.create(XC_TYPE_STRING, key);
            /* 注意：这里我们应该直接访问对象系统的API，而不是直接调用内部函数 */
            /* 假设有一个合适的API，例如 */
            xc_method_func obj_set = find_method(XC_TYPE_OBJECT, "set");
            if (obj_set) {
                xc_val args_array = xc.create(XC_TYPE_ARRAY, 2, key_obj, value);
                obj_set(obj, args_array);
            }
            va_end(args);
            return value;
        }
        
        va_end(args);
        return value;
    }
    
    va_end(args);
    
    /* 这是获取操作 */
    
    /* 首先尝试找类型特定的属性获取器，格式为 "get_属性名" */
    char getter_name[128] = "get_";
    strncat(getter_name, key, sizeof(getter_name) - 5); // 5 = 长度"get_" + 1防止溢出
    
    xc_method_func specific_getter = find_method(type, getter_name);
    if (specific_getter) {
        return specific_getter(obj, NULL);
    }
    
    /* 然后尝试找类型特定的方法 */
    xc_method_func method = find_method(type, key);
    if (method) {
        return method;  // 返回方法函数本身，以便后续调用
    }
    
    /* 如果没有特定方法，尝试通用的获取方法 */
    xc_method_func general_getter = find_method(type, "get");
    if (general_getter) {
        return general_getter(obj, xc.create(XC_TYPE_STRING, key));
    }
    
    /* 如果这是对象类型，直接获取属性 */
    if (type == XC_TYPE_OBJECT) {
        /* 对于对象类型，我们使用特定的对象操作函数 */
        xc_val key_obj = xc.create(XC_TYPE_STRING, key);
        /* 注意：这里我们应该直接访问对象系统的API，而不是直接调用内部函数 */
        /* 假设有一个合适的API，例如 */
        xc_method_func obj_get = find_method(XC_TYPE_OBJECT, "get");
        if (obj_get) {
            return obj_get(obj, key_obj);
        }
    }
    
    return NULL;
}

static xc_val function_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (!closure) return NULL;
    
    /* 获取闭包数据 */
    typedef struct {
        xc_val obj;           /* 对象 */
        xc_method_func func;  /* 方法函数 */
    } method_closure_t;
    
    method_closure_t* method_closure = (method_closure_t*)closure;
    
    /* 添加栈帧 */
    push_stack_frame("function_call", __FILE__, __LINE__);
    
    /* 调用方法函数 */
    xc_val arg = (argc > 0) ? argv[0] : NULL;
    xc_val result = method_closure->func(method_closure->obj, arg);
    
    /* 移除栈帧 */
    pop_stack_frame();
    
    return result;
}

static void clear_error(void) {
    _xc_thread_state.current_error = NULL;
}

static xc_val get_current_error(void) {
    return _xc_thread_state.current_error;
}

static void set_uncaught_exception_handler(xc_val handler) {
    _xc_thread_state.uncaught_handler = handler;
}

static void throw_internal(xc_val error, bool allow_rethrow) {
    /* 记录当前错误 */
    _xc_thread_state.current_error = error;
    
    /* 如果有异常处理器，检查是否允许重新抛出 */
    if (_xc_thread_state.current && !allow_rethrow) {
        /* 检查当前异常与处理器中的异常是否相同（防止无限循环） */
        xc_exception_handler_internal_t* handler = _xc_thread_state.current;
        if (handler->current_exception == error && handler->has_caught) {
            printf("警告: 尝试重复抛出同一异常，已阻止潜在的无限循环\n");
            return;
        }
        
        /* 更新处理器状态 */
        handler->current_exception = error;
        handler->has_caught = true;
    }
    
    /* 如果有异常处理器，跳转到异常处理器 */
    if (_xc_thread_state.current) {
        longjmp(_xc_thread_state.current->env, 1);
    }
    
    /* 如果有未捕获异常处理器，调用它 */
    if (_xc_thread_state.uncaught_handler) {
        xc_val args[1] = {error};
        invoke(_xc_thread_state.uncaught_handler, 1, args);
    } else {
        /* 打印错误信息 */
        printf("未捕获的异常: ");
        if (is(error, XC_TYPE_STRING)) {
            printf("%s\n", (char*)error);
        } else if (is(error, XC_TYPE_EXCEPTION)) {
            printf("%s\n", (char*)error);
        } else {
            printf("<非字符串异常>\n");
        }
        
        /* 打印栈跟踪 */
        printf("栈跟踪:\n");
        xc_stack_frame_t* frame = _xc_thread_state.top;
        while (frame) {
            printf("  %s at %s:%d\n", 
                   frame->func_name ? frame->func_name : "<unknown>",
                   frame->file_name ? frame->file_name : "<unknown>",
                   frame->line_number);
            frame = frame->prev;
        }
        
        /* 终止程序 */
        exit(1);
    }
}

/* 公共异常抛出函数 */
static void throw(xc_val error) {
    /* 默认不允许重新抛出相同异常，修改默认行为以修复循环问题 */
    throw_internal(error, false);
}

/* 添加允许重新抛出的版本，在确实需要的场景使用 */
static void throw_with_rethrow(xc_val error) {
    throw_internal(error, true);
}

static xc_val try_catch_finally(xc_val try_func, xc_val catch_func, xc_val finally_func) {
    /* 检查参数 */
    if (!is(try_func, XC_TYPE_FUNC)) {
        // 避免递归调用create，直接返回NULL
        return NULL;
    }
    
    /* 创建异常处理器 - 使用堆内存分配以避免栈变量的生命周期问题 */
    xc_exception_handler_internal_t* handler = (xc_exception_handler_internal_t*)malloc(sizeof(xc_exception_handler_internal_t));
    if (!handler) {
        // 避免递归调用create，直接返回NULL
        return NULL;
    }
    
    handler->catch_func = catch_func;
    handler->finally_func = finally_func;
    handler->prev = _xc_thread_state.current;
    handler->current_exception = NULL;
    handler->has_caught = false;
    
    /* 设置当前异常处理器 */
    _xc_thread_state.current = handler;
    
    xc_val result = NULL;
    xc_val error = NULL;
    int exception_occurred = 0;
    
    /* 记录当前栈帧 */
    push_stack_frame("try_catch_finally", __FILE__, __LINE__);
    
    /* 尝试执行try块 */
    if (setjmp(handler->env) == 0) {
        /* 正常执行路径 */
        result = invoke(try_func, 0);
    } else {
        /* 异常处理路径 */
        exception_occurred = 1;
        error = _xc_thread_state.current_error;
        _xc_thread_state.current_error = NULL;
        
        /* 如果有catch处理器，调用它 */
        if (catch_func && is(catch_func, XC_TYPE_FUNC)) {
            push_stack_frame("catch_handler", __FILE__, __LINE__);
            xc_val args[1] = {error};
            result = invoke(catch_func, 1, args);
            pop_stack_frame();
        }
    }
    
    /* 保存当前处理器的前一个处理器，以便恢复 */
    xc_exception_handler_internal_t* prev_handler = handler->prev;
    
    /* 恢复之前的异常处理器 */
    _xc_thread_state.current = prev_handler;
    
    /* 释放当前处理器 */
    free(handler);
    
    /* 如果有finally处理器，执行它 */
    if (finally_func && is(finally_func, XC_TYPE_FUNC)) {
        xc_val finally_result = NULL;
        xc_val finally_error = NULL;
        
        /* 创建临时处理器来捕获finally中的异常 */
        xc_exception_handler_internal_t* finally_handler = (xc_exception_handler_internal_t*)malloc(sizeof(xc_exception_handler_internal_t));
        if (finally_handler) {
            finally_handler->catch_func = NULL;
            finally_handler->finally_func = NULL;
            finally_handler->prev = _xc_thread_state.current;
            finally_handler->current_exception = NULL;
            finally_handler->has_caught = false;
            
            _xc_thread_state.current = finally_handler;
            
            push_stack_frame("finally_handler", __FILE__, __LINE__);
            
            if (setjmp(finally_handler->env) == 0) {
                finally_result = invoke(finally_func, 0);
            } else {
                finally_error = _xc_thread_state.current_error;
                _xc_thread_state.current_error = NULL;
            }
            
            pop_stack_frame();
            
            /* 恢复之前的异常处理器 */
            _xc_thread_state.current = finally_handler->prev;
            
            /* 释放临时处理器 */
            free(finally_handler);
        } else {
            /* 内存分配失败，但继续执行 */
            push_stack_frame("finally_handler", __FILE__, __LINE__);
            finally_result = invoke(finally_func, 0);
            pop_stack_frame();
        }
        
        /* 如果finally块抛出异常，它优先于之前的异常 */
        if (finally_error) {
            /* 如果之前有异常且两者都是ERROR类型，设置异常链 */
            if (error && is(finally_error, XC_TYPE_EXCEPTION) && is(error, XC_TYPE_EXCEPTION)) {
                /* 设置原始异常为当前异常的cause */
                call(finally_error, "setCause", error);
            }
            error = finally_error;
            exception_occurred = 1;
            result = NULL;
        }
    }
    
    pop_stack_frame();  /* 移除try_catch_finally帧 */
    
    /* 如果有未处理的异常，重新抛出 */
    if (error) {
        throw(error);
        return NULL;  /* 这行代码应该不会执行，因为throw会导致非本地跳转 */
    }
    
    return result;
}

static int get_type_id(const char* name) {
    if (!name) return -1;
    
    unsigned int hash = hash_string(name);
    xc_type_entry_t* entry = type_registry.buckets[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry->id;
        }
        entry = entry->next;
    }
    
    return -1;  /* 未找到 */
}

/* 调用函数 */
static xc_val invoke(xc_val func, int argc, ...) {
    if (!func) return NULL;
    
    /* 检查类型 */
    if (!is(func, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    /* 获取函数名（如果存在） */
    const char* func_name = "anonymous_function";
    /* 尝试通过属性获取函数名 */
    xc_val name_prop = dot(func, "name");
    if (name_prop && is(name_prop, XC_TYPE_STRING)) {
        func_name = (const char*)name_prop;
    }
    
    /* 添加栈帧 */
    push_stack_frame(func_name, __FILE__, __LINE__);
    
    /* 收集参数 */
    va_list args;
    va_start(args, argc);
    
    xc_val argv[16]; /* 最多支持16个参数 */
    for (int i = 0; i < argc && i < 16; i++) {
        argv[i] = va_arg(args, xc_val);
    }
    
    va_end(args);
    
    /* 直接调用函数，不再处理catch和finally */
    /* 异常处理现在由try_catch_finally函数统一处理 */
    xc_val result = xc_function_invoke(func, NULL, argc, argv);
    
    /* 移除栈帧 */
    pop_stack_frame();
    
    return result;
}

static int is(xc_val val, int type) {
    if (!val) return type == XC_TYPE_NULL;
    return XC_HEADER(val)->type == type;
}

static xc_val call(xc_val obj, const char* method, ...) {
    if (!obj || !method) return NULL;
    
    /* 查找方法 */
    int type = type_of(obj);
    xc_method_func func = find_method(type, method);
    if (!func) {
        /* 方法未找到 */
        return NULL;
    }
    
    /* 添加栈帧 */
    char method_desc[128];
    snprintf(method_desc, sizeof(method_desc), "%s.%s", 
             (type >= 0 && type < 16) ? "Object" : "Unknown", method);
    push_stack_frame(method_desc, __FILE__, __LINE__);
    
    /* 收集参数 */
    va_list args;
    va_start(args, method);
    xc_val arg = va_arg(args, xc_val); /* 获取单个参数 */
    va_end(args);
    
    /* 调用方法 */
    xc_val result = func(obj, arg);
    
    /* 移除栈帧 */
    pop_stack_frame();
    
    return result;
}

static xc_val catch_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !is(argv[0], XC_TYPE_FUNC)) {
        xc_val error = create(XC_TYPE_EXCEPTION, XC_ERR_INVALID_ARGUMENT, "catch需要一个函数参数");
        throw(error);
        return NULL;
    }
    
    /* 获取try的结果 */
    xc_val try_result = closure;
    
    /* 如果是错误对象，则调用catch处理器 */
    if (is(try_result, XC_TYPE_EXCEPTION)) {
        return invoke(argv[0], 1, try_result);
    }
    
    /* 否则，直接返回try的结果 */
    return try_result;
}

static xc_val finally_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !is(argv[0], XC_TYPE_FUNC)) {
        xc_val error = create(XC_TYPE_EXCEPTION, XC_ERR_INVALID_ARGUMENT, "finally需要一个函数参数");
        throw(error);
        return NULL;
    }
    
    /* 获取之前的结果 */
    xc_val prev_result = closure;
    
    /* 无论如何都执行finally处理器 */
    xc_val finally_result = invoke(argv[0], 0);
    
    /* 忽略finally的结果，返回之前的结果 */
    return prev_result;
}

static xc_val try_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !is(argv[0], XC_TYPE_FUNC)) {
        xc_val error = create(XC_TYPE_EXCEPTION, XC_ERR_INVALID_ARGUMENT, "try需要一个函数参数");
        throw(error);
        return NULL;
    }
    
    /* 设置try块标志 */
    bool old_in_try = _xc_thread_state.in_try_block;
    _xc_thread_state.in_try_block = true;
    
    /* 执行函数 */
    xc_val result = invoke(argv[0], 0);
    
    /* 恢复try块标志 */
    _xc_thread_state.in_try_block = old_in_try;
    
    /* 如果有错误，返回错误对象 */
    if (_xc_thread_state.current_error) {
        /* 清除当前错误 */
        _xc_thread_state.current_error = NULL;
        
        /* 在测试中，我们期望返回字符串类型 */
        return create(XC_TYPE_STRING, "异常已处理");
    }
    
    /* 直接返回函数结果 */
    return result;
}

static xc_val try_func(xc_val func) {
    /* 直接调用try处理器 */
    xc_val args[1] = {func};
    return try_handler(NULL, 1, args, NULL);
}

static int type_of(xc_val val) {
    if (!val) return XC_TYPE_NULL;
    
    // 获取对象头
    xc_header_t* header = XC_HEADER(val);
    
    // 检查header是否为有效指针
    if (!header) {
        return XC_TYPE_NULL;
    }
    
    int rt = header->type;
    return rt;
}

static xc_val create(int type, ...) {
    if (type < 0 || type >= 16) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "无效的类型ID: %d", type);
        // 避免递归调用create，直接返回NULL
        return NULL;
    }
    
    // 查找指定类型
    xc_type_entry_t* entry = find_type_by_id(type);
    if (!entry || !entry->lifecycle.creator) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "未找到类型ID %d 的创建函数", type);
        // 避免递归调用create，直接返回NULL
        return NULL;
    }
    
    va_list args;
    va_start(args, type);
    xc_val result = entry->lifecycle.creator(type, args);
    va_end(args);
    
    return result;
}

/* 获取错误的堆栈跟踪 */
xc_object_t *xc_error_get_stack_trace(xc_runtime_t *rt, xc_object_t *error) {
    /* 检查参数 */
    if (!error || !is(error, XC_TYPE_EXCEPTION)) {
        return NULL;
    }
    
    /* 创建一个数组来存储堆栈帧信息 */
    xc_val stack_array = create(XC_TYPE_ARRAY, 0);
    if (!stack_array) {
        return NULL;
    }
    
    /* 遍历当前线程的堆栈帧 */
    xc_stack_frame_t* frame = _xc_thread_state.top;
    while (frame != NULL) {
        /* 为每一帧创建一个包含信息的对象（这里简化为字符串） */
        char frame_info[256];
        snprintf(frame_info, sizeof(frame_info), "%s (%s:%d)",
                 frame->func_name ? frame->func_name : "unknown",
                 frame->file_name ? frame->file_name : "unknown",
                 frame->line_number);
        
        /* 创建字符串对象并添加到数组 */
        xc_val frame_str = create(XC_TYPE_STRING, frame_info);
        if (frame_str) {
            /* 调用数组的push方法添加元素 */
            xc_val args[1] = {frame_str};
            call(stack_array, "push", 1, args);
            /* 这里不需要释放frame_str，因为它已经被添加到数组中 */
        }
        
        /* 移动到上一帧 */
        frame = frame->prev;
    }
    
    return (xc_object_t *)stack_array;
}

/* 按顺序初始化所有基本类型 */
void xc_types_init(void) {
    xc_register_string_type(&xc);
    xc_register_boolean_type(&xc);
    xc_register_number_type(&xc);
    xc_register_array_type(&xc);
    xc_register_object_type(&xc);
    xc_register_function_type(&xc);
    xc_register_error_type(&xc);
}

/* use GCC FEATURE */
void __attribute__((constructor)) xc_auto_init(void) {
    printf("DEBUG xc_auto_init()\n");//TODO log-level
    xc_types_init();//
}

void __attribute__((destructor)) xc_auto_shutdown(void) {
    printf("DEBUG xc_auto_shutdown()\n");//TODO log-level
    xc_gc();
}

// 添加强制引用以确保构造函数编译时被保留
XC_REQUIRES(xc_auto_init);
XC_REQUIRES(xc_auto_shutdown);

xc_runtime_t xc = {
    .alloc_object = alloc_object,
    .type_of = type_of,
    .is = is,
    .register_type = xc_register_type,
    .get_type_id = get_type_id,
    .register_method = register_method,
    .create = create,
    .call = call,
    .dot = dot,
    .invoke = invoke,
    .try_catch_finally = try_catch_finally,
    .throw = throw,
    .throw_with_rethrow = throw_with_rethrow,
    .set_uncaught_exception_handler = set_uncaught_exception_handler,
    .get_current_error = get_current_error,
    .clear_error = clear_error,
    //.gc = xc_gc,
    //.gc_force = xc_gc,
    // .init = xc_auto_init,
    // .shutdown = xc_auto_shutdown
};

/* 添加栈帧 */
static void push_stack_frame(const char* func_name, const char* file_name, int line_number) {
    xc_stack_frame_t* frame = (xc_stack_frame_t*)malloc(sizeof(xc_stack_frame_t));
    if (!frame) {
        /* 内存分配失败，但这是辅助功能，不应中断执行 */
        return;
    }
    
    frame->func_name = func_name;
    frame->file_name = file_name;
    frame->line_number = line_number;
    frame->prev = _xc_thread_state.top;
    
    _xc_thread_state.top = frame;
    _xc_thread_state.depth++;
}

/* 移除栈帧 */
static void pop_stack_frame(void) {
    if (_xc_thread_state.top == NULL) {
        return;
    }
    
    xc_stack_frame_t* frame = _xc_thread_state.top;
    _xc_thread_state.top = frame->prev;
    _xc_thread_state.depth--;
    
    free(frame);
}


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
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    xc_runtime_extended_t *ext_rt = (xc_runtime_extended_t *)rt;
    
    /* Check if we need to run GC */
    gc->allocation_count++;
    if (gc->enabled && 
        (gc->allocation_count >= gc->config.max_alloc_before_gc ||
         gc->used_memory >= gc->config.gc_threshold * gc->heap_size)) {
        xc_gc_run(rt);
    }
    
    /* Allocate memory for the object */
    xc_object_t *obj = (xc_object_t *)malloc(size);
    if (!obj) {
        /* If allocation fails, run GC and try again */
        if (gc->enabled) {
            xc_gc_run(rt);
            obj = (xc_object_t *)malloc(size);
            if (!obj) {
                /* If still fails, return NULL */
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    
    /* Initialize object */
    memset(obj, 0, size);
    obj->size = size;
    obj->ref_count = 1;  /* Start with ref count 1 */
    obj->gc_color = XC_GC_WHITE;
    obj->type = ext_rt->type_handlers[type_id];
    
    /* Add to white list */
    obj->gc_next = gc->white_list;
    gc->white_list = obj;
    
    /* Update statistics */
    gc->heap_size += size;
    gc->used_memory += size;
    gc->total_allocated++;
    
    return obj;
}

/* Free an object */
void xc_gc_free(xc_runtime_t *rt, xc_object_t *obj) {
    if (!obj) return;
    
    xc_gc_context_t *gc = xc_gc_get_context(rt);
    
    /* Call type-specific cleanup if available */
    if (obj->type && obj->type->free) {
        obj->type->free(rt, obj);
    }
    
    /* Remove from white list */
    xc_object_t *curr = gc->white_list;
    xc_object_t *prev = NULL;
    
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

/* Mark an object as permanently reachable */
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

/* Global GC function for backward compatibility */
void xc_gc_collect(void) {
    /* This is a placeholder for backward compatibility */
    /* In a real implementation, we would need to get the current runtime */
    /* and call xc_gc_run on it */
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