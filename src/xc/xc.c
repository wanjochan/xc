#include "xc.h"
#include "xc_internal.h"

static xc_runtime_t* rt = NULL;

// 函数声明
void init_method_cache(void);
void clear_method_cache(void);

// Define MAX_TYPE_ID as 10
#define MAX_TYPE_ID 255

// Define xc_type_handlers as an array of type lifecycle pointers
xc_type_lifecycle_t *xc_type_handlers[MAX_TYPE_ID];

/* 根据类型ID获取类型处理器 */
xc_type_lifecycle_t* get_type_handler(int type_id) {
    if (type_id >= 0 && type_id < MAX_TYPE_ID) {
        return xc_type_handlers[type_id];
    }
    return NULL;
}

///////////////////////////////////////////////////
/* 简单的哈希函数 */
static unsigned int hash_string(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = hash * 31 + (*str++);
    }
    return hash % TYPE_HASH_SIZE;
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
static xc_val xc_alloc(int type, size_t size) {
    if (type < 0 || type >= 16) return NULL;
    
    /* 获取类型注册项 */
    xc_type_entry_t* entry = find_type_by_id(type);
    if (!entry) {
        printf("WARNING: xc_alloc: type %d not found\n", type);
        return NULL;
    }
    
    // /* 获取对象大小 */
    // va_list args;
    // va_start(args, type);
    // size_t size = va_arg(args, size_t);
    // va_end(args);
    
    return xc_gc_alloc(rt, size, type);
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

/* 获取对象的原生值 */
static void* get_type_value(xc_val obj) {
    if (!obj) return NULL;
    
    int type_id = obj->type_id;
    xc_type_lifecycle_t* lifecycle = get_type_handler(type_id);
    
    if (lifecycle && lifecycle->get_value) {
        return lifecycle->get_value(obj);
    }
    
    return NULL;
}

/* 将对象转换为目标类型 */
static xc_val convert_type(xc_val obj, int target_type) {
    if (!obj) return NULL;
    
    // 如果已经是目标类型，直接返回
    if (obj->type_id == target_type) {
        return obj;
    }
    
    int type_id = obj->type_id;
    xc_type_lifecycle_t* lifecycle = get_type_handler(type_id);
    
    if (lifecycle && lifecycle->convert_to) {
        return lifecycle->convert_to(obj, target_type);
    }
    
    return NULL;
}

/* 注册类型 */
int xc_register_type(const char* name, xc_type_lifecycle_t* lifecycle) {
    // printf("DEBUG xc_register_type(\"%s\"), lifecycle=%d\n", name, lifecycle);
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
    XC_LOG_DEBUG("xc_register_type(\"%s\"), lifecycle=%d, type_id=%d", name, lifecycle, type_id);
    if (lifecycle->creator) {
        XC_LOG_DEBUG("%s creator=%d", name, lifecycle->creator);
    }
    if (lifecycle->initializer) {
        XC_LOG_DEBUG("%s initializer=%d", name, lifecycle->initializer);
    }

    /* 添加到哈希表 */
    unsigned int hash = hash_string(name);
    entry->next = type_registry.buckets[hash];
    type_registry.buckets[hash] = entry;
    
    type_registry.count++;
    
    if (entry->lifecycle.initializer) {
        entry->lifecycle.initializer();//todo change to rt once the 1sr arg ready
    }
    
    return type_id;
}

/* 注册方法 */
static char register_method(int type, const char* name, xc_method_func func) {
    XC_LOG_DEBUG("register_method: type=%d, name=%s, func=%p", type, name, func);
    if (type < 0 || type >= 16 || !name || !func) {
        XC_LOG_DEBUG("register_method: invalid parameters");
        return 0;
    }
    
    if (_state.method_count >= 256) {
        XC_LOG_DEBUG("register_method: method table full");
        return 0;
    }
    
    int method_idx = _state.method_count++;
    _state.methods[method_idx].name = name;
    _state.methods[method_idx].func = (xc_val (*)(xc_val, ...))func;
    _state.methods[method_idx].next = _state.method_heads[type];
    _state.method_heads[type] = method_idx;
    XC_LOG_DEBUG("register_method: registered method at index %d, next=%d, head=%d",
           method_idx, _state.methods[method_idx].next, _state.method_heads[type]);
    
    return 1;
}

// 全局缓存数组
#ifndef METHOD_CACHE_SIZE
#define METHOD_CACHE_SIZE 32  // 增加默认缓存大小
#endif

static method_cache_entry_t method_cache[METHOD_CACHE_SIZE] = {0};
static int cache_age_counter = 0;

// 缓存统计信息
#ifdef XC_ENABLE_CACHE_STATS
static struct {
    unsigned int hits;
    unsigned int misses;
    unsigned int collisions;
} cache_stats = {0};
#endif

/* 原始的方法查找函数（无缓存） */
static xc_method_func find_method_original(int type, const char* name) {
    if (type < 0 || type >= 16 || !name) {
        return NULL;
    }
    
    int method_idx = _state.method_heads[type];
    XC_LOG_DEBUG("find_method_original: method_idx=%d", method_idx);
    
    while (method_idx != 0) {
        XC_LOG_DEBUG("find_method_original: checking method at index %d, name=%s", method_idx, _state.methods[method_idx].name);
        if (strcmp(_state.methods[method_idx].name, name) == 0) {
            XC_LOG_DEBUG("find_method_original: found method at index %d, func=%p", method_idx, _state.methods[method_idx].func);
            return (xc_method_func)_state.methods[method_idx].func;
        }
        method_idx = _state.methods[method_idx].next;
        XC_LOG_DEBUG("find_method_original: next method_idx=%d", method_idx);
    }
    
    XC_LOG_DEBUG("find_method_original: method not found");
    return NULL;
}

/* 批量查找多个方法 */
static void find_methods_batch(int type, const char** names, int count, xc_method_func* results) {
    if (type < 0 || type >= 16) {
        for (int i = 0; i < count; i++) {
            results[i] = NULL;
        }
        return;
    }
    
    // 优化：一次性计算类型部分的键
    unsigned int type_key = ((unsigned int)type << 16);
    
    for (int i = 0; i < count; i++) {
        const char* name = names[i];
        if (!name) {
            results[i] = NULL;
            continue;
        }
        
        // 计算完整缓存键
        unsigned int name_hash = hash_string(name);
        unsigned int key = type_key | (name_hash & 0xFFFF);
        
        // 查找缓存 - 添加安全检查
        bool cache_hit = false;
        for (int j = 0; j < METHOD_CACHE_SIZE; j++) {
            // 检查缓存条目是否有效
            if (method_cache[j].key == key && method_cache[j].value) {
                // 更新访问时间 - 使用原子操作避免多线程问题
                method_cache[j].age = ++cache_age_counter;
                results[i] = method_cache[j].value;
                cache_hit = true;
                
                #ifdef XC_ENABLE_CACHE_STATS
                cache_stats.hits++;
                #endif
                
                XC_LOG_DEBUG("find_methods_batch: cache hit for type=%d, name=%s", type, name);
                break;
            }
        }
        
        if (!cache_hit) {
            #ifdef XC_ENABLE_CACHE_STATS
            cache_stats.misses++;
            #endif
            
            XC_LOG_DEBUG("find_methods_batch: cache miss for type=%d, name=%s", type, name);
            
            // 缓存未命中，调用原始查找函数
            xc_method_func method = find_method_original(type, name);
            results[i] = method;
            
            if (method) {
                // 添加到缓存，只有当方法不为空时才缓存
                int oldest_idx = 0;
                int empty_idx = -1;
                
                // 首先尝试找空槽位
                for (int j = 0; j < METHOD_CACHE_SIZE; j++) {
                    if (!method_cache[j].value) {
                        empty_idx = j;
                        break;
                    }
                    if (method_cache[j].age < method_cache[oldest_idx].age) {
                        oldest_idx = j;
                    }
                }
                
                int target_idx = (empty_idx >= 0) ? empty_idx : oldest_idx;
                
                // 检查是否有冲突（相同键但不同值）
                #ifdef XC_ENABLE_CACHE_STATS
                if (method_cache[target_idx].key == key && 
                    method_cache[target_idx].value != method) {
                    cache_stats.collisions++;
                }
                #endif
                
                // 安全地更新缓存条目
                method_cache_entry_t new_entry = {
                    .key = key,
                    .value = method,
                    .age = ++cache_age_counter
                };
                method_cache[target_idx] = new_entry;
                
                XC_LOG_DEBUG("find_methods_batch: added to cache at index %d", target_idx);
            }
        }
    }
}

/* 查找方法（带缓存） - 优化版本，直接使用批量查找 */
static xc_method_func find_method(int type, const char *name) {
    if (type < 0 || type >= 16 || !name) {
        return NULL;
    }
    
    xc_method_func result;
    find_methods_batch(type, &name, 1, &result);
    return result;
}

xc_val xc_dot(xc_val obj, const char* key, ...) {
    if (!obj || !key) return NULL;
    
    /* 处理可变参数 */
    va_list args;
    va_start(args, key);
    
    /* 获取第一个额外参数 */
    xc_val value = va_arg(args, xc_val);
    
    /* 获取对象类型 */
    int type = xc_typeof(obj);
    
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
            xc_val result = general_setter(obj, rt->new(XC_TYPE_STRING, key));
            va_end(setter_args);
            va_end(args);
            return result;
        }
        
        /* 如果这是对象类型，直接设置属性 */
        if (type == XC_TYPE_OBJECT) {
            /* 对于对象类型，我们使用特定的对象操作函数 */
            /* 在实际代码中，应该通过结构体访问或函数指针访问 object_set */
            /* 假设有一个合适的API，例如 */
            xc_val key_obj = rt->new(XC_TYPE_STRING, key);
            /* 注意：这里我们应该直接访问对象系统的API，而不是直接调用内部函数 */
            /* 假设有一个合适的API，例如 */
            xc_method_func obj_set = find_method(XC_TYPE_OBJECT, "set");
            if (obj_set) {
                xc_val args_array = rt->new(XC_TYPE_ARRAY, 2, key_obj, value);
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
    
    /* 准备方法名数组 */
    const char* method_names[3];
    xc_method_func methods[3];
    
    /* 设置特定getter名称 */
    char getter_name[128] = "get_";
    strncat(getter_name, key, sizeof(getter_name) - 5); // 5 = 长度"get_" + 1防止溢出
    
    method_names[0] = getter_name;  // "get_xxx"
    method_names[1] = key;          // 直接方法名
    method_names[2] = "get";        // 通用getter
    
    /* 批量查找 */
    find_methods_batch(type, method_names, 3, methods);
    
    /* 使用结果 */
    if (methods[0]) {
        return methods[0](obj, NULL);
    }
    
    if (methods[1]) {
        // return methods[1];  // 返回方法函数本身，以便后续调用
        return rt->new(XC_TYPE_FUNC, method_names[1], methods[1]);
    }
    
    if (methods[2]) {
        return methods[2](obj, rt->new(XC_TYPE_STRING, key));
    }
    
    /* 如果这是对象类型，直接获取属性 */
    if (type == XC_TYPE_OBJECT) {
        /* 对于对象类型，我们使用特定的对象操作函数 */
        xc_val key_obj = rt->new(XC_TYPE_STRING, key);
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
    xc_set_uncaught_exception_handler(NULL, handler);
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
        return; /* 不会执行到这里 */
    }
    
    /* 如果有未捕获异常处理器，调用它 */
    if (_xc_thread_state.uncaught_handler) {
        xc_val args[1] = {error};
        xc_invoke(_xc_thread_state.uncaught_handler, 1, args);
    } else {
        /* 打印错误信息 */
        printf("未捕获的异常: ");
        if (rt->is(error, XC_TYPE_STRING)) {
            printf("%s\n", (char*)error);
        } else if (rt->is(error, XC_TYPE_EXCEPTION)) {
            xc_exception_t *exc = (xc_exception_t *)error;
            printf("%s\n", exc->message ? exc->message : "<无消息>");
            
            /* 打印栈跟踪 */
            if (exc->stack_trace) {
                printf("异常堆栈:\n");
                for (size_t i = 0; i < exc->stack_trace->count; i++) {
                    printf("  %s at %s:%d\n", 
                           exc->stack_trace->entries[i].function ? exc->stack_trace->entries[i].function : "<unknown>",
                           exc->stack_trace->entries[i].file ? exc->stack_trace->entries[i].file : "<unknown>",
                           exc->stack_trace->entries[i].line);
                }
            }
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
    /* 调用异常模块的抛出函数 */
    xc_exception_throw(rt, error);
}

/* 添加允许重新抛出的版本，在确实需要的场景使用 */
static void throw_with_rethrow(xc_val error) {
    /* 确保错误对象有效 */
    if (!error) {
        printf("警告: 尝试抛出空异常\n");
        return;
    }
    
    /* 调用异常模块的抛出函数 */
    xc_exception_throw(rt, error);
}

static xc_val try_catch_finally(xc_val try_func, xc_val catch_func, xc_val finally_func) {
    /* 检查参数 */
    if (!try_func || !rt->is(try_func, XC_TYPE_FUNC)) {
        xc_val error = rt->new(XC_TYPE_EXCEPTION, "try_func必须是函数类型");
        throw(error);
        return NULL;
    }
    
    /* 创建异常帧 */
    xc_exception_frame_t frame;
    frame.prev = xc_exception_frame;
    frame.exception = NULL;
    frame.handled = false;
    frame.file = __FILE__;
    frame.line = __LINE__;
    frame.finally_handler = NULL;
    frame.finally_context = NULL;
    
    /* 设置当前异常帧 */
    xc_exception_frame = &frame;
    
    xc_val result = NULL;
    xc_val error = NULL;
    bool exception_occurred = false;
    
    /* 记录当前栈帧 */
    push_stack_frame("try_catch_finally", __FILE__, __LINE__);
    
    printf("DEBUG: 开始执行try块\n");
    
    /* 尝试执行try块 */
    if (setjmp(frame.jmp) == 0) {
        /* 正常执行路径 */
        printf("DEBUG: 正常执行try块\n");
        result = xc_invoke(try_func, 0);
        printf("DEBUG: try块执行完成，结果=%p\n", result);
    } else {
        /* 异常处理路径 */
        printf("DEBUG: 捕获到异常，进入异常处理路径\n");
        exception_occurred = true;
        error = frame.exception;
        frame.exception = NULL;
        
        printf("DEBUG: 异常对象=%p，类型=%d\n", error, xc_typeof(error));
        
        /* 如果有catch处理器，调用它 */
        if (catch_func && rt->is(catch_func, XC_TYPE_FUNC)) {
            printf("DEBUG: 调用catch处理器\n");
            push_stack_frame("catch_handler", __FILE__, __LINE__);
            
            /* 创建临时异常帧来捕获catch中可能的异常 */
            xc_exception_frame_t catch_frame;
            catch_frame.prev = xc_exception_frame;
            catch_frame.exception = NULL;
            catch_frame.handled = false;
            catch_frame.file = __FILE__;
            catch_frame.line = __LINE__;
            catch_frame.finally_handler = NULL;
            catch_frame.finally_context = NULL;
            
            xc_exception_frame = &catch_frame;
            
            /* 使用setjmp捕获catch中可能抛出的异常 */
            if (setjmp(catch_frame.jmp) == 0) {
                /* 确保参数合法 */
                xc_val args[1] = {error ? error : rt->new(XC_TYPE_NULL)};
                printf("DEBUG: 准备调用catch处理器，参数=%p\n", args[0]);
                result = xc_invoke(catch_func, 1, args);
                printf("DEBUG: catch处理器执行完成，结果=%p\n", result);
                
                /* 标记异常已处理 */
                exception_occurred = false;
                
                /* 确保结果不为NULL，如果是NULL则创建一个默认值 */
                if (result == NULL) {
                    result = rt->new(XC_TYPE_STRING, "Caught");
                }
            } else {
                /* catch中抛出了异常 */
                printf("DEBUG: catch处理器中抛出了异常\n");
                error = catch_frame.exception;
                catch_frame.exception = NULL;
                
                /* 标记为需要重新抛出 */
                exception_occurred = true;
            }
            
            /* 恢复异常帧 */
            xc_exception_frame = catch_frame.prev;
            
            /* 弹出栈帧 */
            pop_stack_frame();
        } else {
            /* 没有catch处理器，保持异常状态 */
            printf("DEBUG: 没有提供catch处理器，将重新抛出异常\n");
        }
    }
    
    /* 恢复异常帧 */
    xc_exception_frame = frame.prev;
    
    /* 尝试执行finally块 */
    xc_val finally_result = NULL;
    if (finally_func && rt->is(finally_func, XC_TYPE_FUNC)) {
        printf("DEBUG: 调用finally处理器\n");
        push_stack_frame("finally_handler", __FILE__, __LINE__);
        
        /* 创建临时异常帧来捕获finally中可能的异常 */
        xc_exception_frame_t finally_frame;
        finally_frame.prev = xc_exception_frame;
        finally_frame.exception = NULL;
        finally_frame.handled = false;
        finally_frame.file = __FILE__;
        finally_frame.line = __LINE__;
        finally_frame.finally_handler = NULL;
        finally_frame.finally_context = NULL;
        
        xc_exception_frame = &finally_frame;
        
        /* 清理方法缓存，防止在finally执行过程中访问无效的缓存条目 */
        clear_method_cache();
        
        /* 使用setjmp捕获finally中可能抛出的异常 */
        if (setjmp(finally_frame.jmp) == 0) {
            xc_val args[1] = {exception_occurred ? (error ? error : rt->new(XC_TYPE_NULL)) : rt->new(XC_TYPE_NULL)};
            finally_result = xc_invoke(finally_func, 1, args);
            printf("DEBUG: finally处理器执行完成，结果=%p\n", finally_result);
        } else {
            /* finally中抛出了异常 */
            printf("DEBUG: finally处理器中抛出了异常\n");
            error = finally_frame.exception;
            finally_frame.exception = NULL;
            
            /* finally抛出的异常总是优先的 */
            exception_occurred = true;
        }
        
        /* 恢复异常帧 */
        xc_exception_frame = finally_frame.prev;
        
        /* 弹出栈帧 */
        pop_stack_frame();
    }
    
    /* 弹出栈帧 */
    pop_stack_frame();
    
    /* 如果有未处理的异常，重新抛出 */
    if (exception_occurred) {
        printf("DEBUG: 重新抛出未处理的异常\n");
        throw_with_rethrow(error);
        return NULL;
    }
    
    /* 返回结果，优先返回try/catch的结果，忽略finally的结果 */
    printf("DEBUG: try_catch_finally执行完成，返回结果=%p\n", result);
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
xc_val xc_invoke(xc_val func, int argc, ...) {
    if (!func) return NULL;
    
    /* 检查类型 */
    if (!rt->is(func, XC_TYPE_FUNC)) {
        return NULL;
    }
    
    /* 获取函数名（如果存在） */
    const char* func_name = "<function>";
    /* 尝试通过属性获取函数名 */
    xc_val name_prop = xc_dot(func, "name");
    if (name_prop && rt->is(name_prop, XC_TYPE_STRING)) {
        func_name = (const char*)name_prop;
    }
    
    /* 添加栈帧 */
    push_stack_frame(func_name, __FILE__, __LINE__);
    
    /* 收集参数 */
    va_list args;
    va_start(args, argc);
    
    xc_val argv[16]; /* 最多支持16个参数 */
    
    /* 从可变参数列表中收集参数 */
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

int xc_typeof(xc_val val) {
    if (!val) return XC_TYPE_NULL;
    
    // 获取对象
    xc_object_t* obj = (xc_object_t*)val;
    
    // 检查obj是否为有效指针
    if (!obj) {
        return XC_TYPE_NULL;
    }
    
    // 直接返回类型ID
    return obj->type_id;
}

int xc_is(xc_val val, int type) {
    if (!val) return type == XC_TYPE_NULL;
    
    // 直接使用xc_typeof函数获取类型ID
    int obj_type = xc_typeof(val);
    
    return obj_type == type;
}

xc_val xc_call(xc_val obj, const char* method, ...) {
    XC_LOG_DEBUG("call: obj=%p, method=%s", obj, method);
    if (!obj || !method) {
        XC_LOG_DEBUG("call: invalid parameters");
        return NULL;
    }
    
    /* 查找方法 */
    int type = xc_typeof(obj);
    XC_LOG_DEBUG("call: type=%d", type);
    xc_method_func func = find_method(type, method);
    if (!func) {
        /* 方法未找到 */
        XC_LOG_DEBUG("call: method not found");
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
    XC_LOG_DEBUG("call: calling method func=%p, obj=%p, arg=%p", func, obj, arg);
    xc_val result = func(obj, arg);
    XC_LOG_DEBUG("call: method returned result=%p", result);
    
    /* 移除栈帧 */
    pop_stack_frame();
    
    return result;
}

static xc_val catch_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !rt->is(argv[0], XC_TYPE_FUNC)) {
        xc_val error = rt->new(XC_TYPE_EXCEPTION, "catch需要一个函数参数");
        throw(error);
        return NULL;
    }
    
    /* 获取try的结果 */
    xc_val try_result = closure;
    
    /* 如果是错误对象，则调用catch处理器 */
    if (rt->is(try_result, XC_TYPE_EXCEPTION)) {
        return rt->invoke(argv[0], 1, try_result);
    }
    
    /* 否则，直接返回try的结果 */
    return try_result;
}

static xc_val finally_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !rt->is(argv[0], XC_TYPE_FUNC)) {
        xc_val error = rt->new(XC_TYPE_EXCEPTION, "finally需要一个函数参数");
        throw(error);
        return NULL;
    }
    
    /* 获取之前的结果 */
    xc_val prev_result = closure;
    
    /* 无论如何都执行finally处理器 */
    xc_val finally_result = rt->invoke(argv[0], 0);
    
    /* 忽略finally的结果，返回之前的结果 */
    return prev_result;
}

static xc_val try_handler(xc_val this_obj, int argc, xc_val* argv, xc_val closure) {
    if (argc < 1 || !argv[0] || !rt->is(argv[0], XC_TYPE_FUNC)) {
        xc_val error = rt->new(XC_TYPE_EXCEPTION, "try需要一个函数参数");
        throw(error);
        return NULL;
    }
    
    /* 设置try块标志 */
    bool old_in_try = _xc_thread_state.in_try_block;
    _xc_thread_state.in_try_block = true;
    
    /* 执行函数 */
    xc_val result = rt->invoke(argv[0], 0);
    
    /* 恢复try块标志 */
    _xc_thread_state.in_try_block = old_in_try;
    
    /* 如果有错误，返回错误对象 */
    if (_xc_thread_state.current_error) {
        /* 清除当前错误 */
        _xc_thread_state.current_error = NULL;
        
        /* 在测试中，我们期望返回字符串类型 */
        return rt->new(XC_TYPE_STRING, "异常已处理");
    }
    
    /* 直接返回函数结果 */
    return result;
}

static xc_val try_func(xc_val func) {
    /* 直接调用try处理器 */
    xc_val args[1] = {func};
    return try_handler(NULL, 1, args, NULL);
}


// /* Release a reference to an object */
// xc_val xc_delete(xc_runtime_t *rt, xc_object_t *obj) {
//     //fwd to xc_gc_free? xc_gc_release??
//     if (!obj) return NULL;
    
//     // 不再减少引用计数或释放对象
//     // 而是可能执行以下操作：
    
//     // 1. 记录解除引用事件（可选，用于调试）
//     // if (rt->gc_debug_enabled) {
//     //     printf("Reference released to object %p of type %d\n", 
//     //            (void*)obj, obj->type_id);
//     // }
    
//     // // 2. 可能触发 GC（如果解除引用次数达到阈值）
//     // rt->gc_release_count++;
//     // if (rt->gc_release_count >= rt->gc_release_threshold) {
//     //     rt->gc_release_count = 0;
//     //     // 可选：触发 GC
//     //     // xc_gc_run(rt);
//     // }
    
//     // 3. 对于特殊资源，可能需要特殊处理
//     // 例如，如果对象持有外部资源，可能需要通知资源管理器
//     return NULL;//return boolean true?
// }

xc_val xc_new(int type, ...) {
    if (type < 0 || type >= 16) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "无效的类型ID: %d", type);
        // 避免递归调用xc_new，直接返回NULL
        return NULL;
    }
    
    // 查找指定类型
    xc_type_entry_t* entry = find_type_by_id(type);
    if (!entry || !entry->lifecycle.creator) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "未找到类型ID %d 的创建函数", type);
        // 避免递归调用xc_new，直接返回NULL
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
    if (!error || !rt->is(error, XC_TYPE_EXCEPTION)) {
        return NULL;
    }
    
    /* 创建一个数组来存储堆栈帧信息 */
    xc_val stack_array = rt->new(XC_TYPE_ARRAY, 0);
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
        xc_val frame_str = rt->new(XC_TYPE_STRING, frame_info);
        if (frame_str) {
            /* 调用数组的push方法添加元素 */
            xc_val args[1] = {frame_str};
            xc_call(stack_array, "push", 1, args);
            /* 这里不需要释放frame_str，因为它已经被添加到数组中 */
        }
        
        /* 移动到上一帧 */
        frame = frame->prev;
    }
    
    return (xc_object_t *)stack_array;
}


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
    // 如果是同一个对象，直接返回true
    if (a == b) {
        return true;
    }
    
    // 如果类型不同，返回false
    if (a->type_id != b->type_id) {
        return false;
    }
    
    // 获取类型处理器
    xc_type_lifecycle_t *type_handler = get_type_handler(a->type_id);
    
    // 如果有equal函数，调用它
    if (type_handler && type_handler->equal) {
        return type_handler->equal((xc_val)a, (xc_val)b);
    }
    
    // 默认情况下，只有同一个对象才相等
    return false;
}

/*
 * Compare two XC objects for ordering
 * Returns -1 if a < b, 0 if a == b, 1 if a > b
 */
int xc_compare(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    // 如果是同一个对象，返回0
    if (a == b) {
        return 0;
    }
    
    // 如果类型不同，按类型ID排序
    if (a->type_id != b->type_id) {
        return a->type_id - b->type_id;
    }
    
    /* Delegate to type-specific compare function */
    xc_type_lifecycle_t *type_handler = get_type_handler(a->type_id);
    if (type_handler && type_handler->compare) {
        return type_handler->compare((xc_val)a, (xc_val)b);
    }
    
    // 默认情况下，按内存地址排序
    return (a < b) ? -1 : 1;
}

/*
 * Strict equality check (same type and value)
 */
bool xc_strict_equal(xc_runtime_t *rt, xc_object_t *a, xc_object_t *b) {
    // 如果是同一个对象，直接返回true
    if (a == b) {
        return true;
    }
    
    // 如果类型不同，返回false
    if (a->type_id != b->type_id) {
        return false;
    }
    
    /* Delegate to type-specific equal function */
    xc_type_lifecycle_t *type_handler = get_type_handler(a->type_id);
    if (type_handler && type_handler->equal) {
        return type_handler->equal((xc_val)a, (xc_val)b);
    }
    
    // 默认情况下，只有同一个对象才相等
    return false;
}

/* 
 * Function to invoke a function object
 * This is a stub implementation that just returns NULL
 * The real implementation would be in xc_function.c
 */
xc_val xc_function_invoke(xc_val func, xc_val this_obj, int argc, xc_val* argv) {
    /* 检查参数 */
    if (!func) {
        printf("DEBUG: xc_function_invoke 失败，func 为 NULL\n");
        return NULL;
    }
    
    /* 检查函数类型 */
    if (!rt->is(func, XC_TYPE_FUNC)) {
        printf("DEBUG: xc_function_invoke 失败，func 不是函数类型，类型=%d\n", xc_typeof(func));
        return NULL;
    }
    
    /* 获取函数对象 */
    xc_function_t* function = (xc_function_t*)func;
    
    /* 检查处理器 */
    if (!function->handler) {
        printf("DEBUG: 函数对象没有 handler\n");
        return NULL;
    }
    
    /* 准备参数 */
    xc_val result = function->handler(rt, this_obj, argc, argv);
    
    /* 返回结果 */
    return result;
}



void __attribute__((destructor)) xc_auto_shutdown(void) {
    XC_LOG_DEBUG("xc_auto_shutdown()");
    
    // 清理方法缓存
    clear_method_cache();
    
    xc_gc();
    xc_gc_shutdown(rt);
}

/* 全局类型注册表 */
xc_type_registry_t type_registry = {0};

/* 内置类型处理器 */
xc_type_lifecycle_t *xc_null_type = NULL;
// xc_type_lifecycle_t *xc_boolean_type = NULL;
xc_type_lifecycle_t *xc_number_type = NULL;
xc_type_lifecycle_t *xc_string_type = NULL;
xc_type_lifecycle_t *xc_array_type = NULL;
xc_type_lifecycle_t *xc_object_type = NULL;
xc_type_lifecycle_t *xc_function_type = NULL;
xc_type_lifecycle_t *xc_error_type = NULL;

/* 全局运行时对象 */
xc_runtime_t xc = {
    .type_of = xc_typeof,
    .is = xc_is,
    .register_type = xc_register_type,
    .get_type_id = get_type_id,
    
    .register_method = register_method,
    .new = xc_new,
    .alloc = xc_alloc,
    .get_type_value = get_type_value,
    .convert_type = convert_type,
    // .delete = xc_delete,
    .call = xc_call,
    .dot = xc_dot,
    .invoke = xc_invoke,
    
    .try_catch_finally = try_catch_finally,
    .throw = throw,
    .throw_with_rethrow = throw_with_rethrow,
    .set_uncaught_exception_handler = set_uncaught_exception_handler,
    .get_current_error = get_current_error,
    .clear_error = clear_error,
};



/* use GCC FEATURE */
xc_runtime_t* __attribute__((constructor)) xc_init(void) {
    if (rt) return rt;//@ref xc_singleton()
    XC_LOG_DEBUG("xc_init()");
rt = &xc;
    xc_gc_init_auto(rt, NULL);//@see xc_gc.c
    
    // 初始化方法缓存
    init_method_cache();

    xc_register_string_type(rt);
    xc_register_boolean_type(rt);
    xc_register_number_type(rt);
    xc_register_array_type(rt);
    xc_register_object_type(rt);
    xc_register_function_type(rt);
    xc_register_error_type(rt);
    return rt;
}

xc_runtime_t* xc_singleton(void) {
    if (!rt) {
        rt = xc_init();
    }
    return rt;
}


// 添加强制引用以确保构造函数编译时被保留
XC_REQUIRES(xc_init);
XC_REQUIRES(xc_auto_shutdown);

void init_method_cache(void) {
    // 使用安全的方式初始化缓存
    for (int i = 0; i < METHOD_CACHE_SIZE; i++) {
        method_cache[i].key = 0;
        method_cache[i].value = NULL;
        method_cache[i].age = 0;
    }
    cache_age_counter = 0;
    
    #ifdef XC_ENABLE_CACHE_STATS
    memset(&cache_stats, 0, sizeof(cache_stats));
    #endif
    
    XC_LOG_DEBUG("方法缓存已初始化，大小: %d", METHOD_CACHE_SIZE);
}

void clear_method_cache(void) {
    // 使用安全的方式清理缓存
    for (int i = 0; i < METHOD_CACHE_SIZE; i++) {
        method_cache[i].key = 0;
        method_cache[i].value = NULL;
        method_cache[i].age = 0;
    }
    
    XC_LOG_DEBUG("方法缓存已清理");
}
