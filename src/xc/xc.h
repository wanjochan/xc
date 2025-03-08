#ifndef XC_H
#define XC_H

//DO NOT change this file until PM approved
//@ref build_libxc.sh: xc.h => libxc.h for libxc.a

#include "cosmopolitan.h" //will change to infrax in later versionss

#define XC_FALSE 0
#define XC_TRUE 1

/* 类型ID定义 */
/* 核心类型区间 */
#define XC_TYPE_UNKNOWN     -2//
#define XC_TYPE_UNDEFINED   -1//
//XC_TYPE_VOID ? how about?
#define XC_TYPE_NULL         2
#define XC_TYPE_BOOL         3
#define XC_TYPE_NUMBER       4
#define XC_TYPE_STRING       5
#define XC_TYPE_EXCEPTION    6
#define XC_TYPE_FUNC         7
#define XC_TYPE_ARRAY        8
#define XC_TYPE_OBJECT       9
#define XC_TYPE_VM           10

/* 保留区间 (16-31)：为未来的基础类型预留 */

/* 内部类型 (32-63)：运行时内部使用的类型 */
#define XC_TYPE_INTERNAL_BEGIN  32
#define XC_TYPE_ITERATOR        32
#define XC_TYPE_REGEXP          33
#define XC_TYPE_DATE            34
#define XC_TYPE_BUFFER          35
#define XC_TYPE_INTERNAL_END    63

/* 用户自定义类型 (64-127)：通过API注册的类型 */
#define XC_TYPE_USER_BEGIN      64
#define XC_TYPE_USER_END        127

/* 扩展类型 (128-255)：为未来扩展预留 */
#define XC_TYPE_EXTENSION_BEGIN 128
#define XC_TYPE_EXTENSION_END   255

/* 胖指针 值类型 */
typedef struct xc_object {
    size_t size;              /* Total size of the object in bytes */
    int type_id;//XC_TYPE_*
    // int ref_count;            /* Reference count for manual memory management */
    int gc_color;             /* GC mark color (white, gray, black, permanent) */
    struct xc_object *gc_next; /* Next object in the GC list */
    /* Object data follows this header */
} xc_object_t;
typedef xc_object_t* xc_val;

typedef struct xc_runtime_t xc_runtime_t;

/* 函数类型定义 */
typedef void (*xc_initializer_func)(void);
typedef void (*xc_cleaner_func)(void);
typedef xc_val (*xc_creator_func)(int type, va_list args);
typedef int (*xc_destroy_func)(xc_val obj);
/*
回调机制的工作原理，想象一下这个过程：
GC 开始标记阶段，从根对象开始
对于每个对象，GC 调用其类型的 marker 函数
GC 提供一个回调函数（mark_func），类型实现使用它标记子对象
类型实现遍历自己的内部结构，对每个子对象调用 mark_func
这个过程递归进行，直到所有可达对象都被标记
这种设计的优势：
解耦合：GC 不需要了解类型内部结构，类型不需要了解 GC 内部实现
扩展性：添加新类型只需实现正确的 marker 函数，不需要修改 GC
灵活性：GC 算法可以改变而不影响类型实现
*/

typedef void (*mark_func)(xc_val);
typedef void (*xc_marker_func)(xc_val, mark_func);
typedef xc_val (*xc_allocator_func)(size_t size);
typedef xc_val (*xc_method_func)(xc_val self, xc_val arg);

/* 类型生命周期管理结构 */
//TODO 考虑合并 xc_type_t
typedef struct {
    xc_initializer_func initializer;  /* 初始化函数 */
    xc_cleaner_func cleaner;          /* 清理函数 */
    xc_creator_func creator;          /* 创建函数 */
    xc_destroy_func destroyer;        /* 销毁函数 */
    //xc_allocator_func allocator;      /* 内存分配函数 */
    xc_marker_func marker;            /* GC标记函数 */
    const char *name;
    bool (*equal)(xc_val a, xc_val b);
    int (*compare)(xc_val a, xc_val b);
    int flags;//保留
} xc_type_lifecycle_t;

/* 运行时接口结构 */
// x = null; 类似于将变量设为 null 值或 undefined
// delete obj.prop; 类似于我们讨论的 xc_delete() 操作
typedef struct xc_runtime_t {
/*
下一步建议
既然清理旧代码解决了问题，我建议：
完成清理工作：
彻底检查并移除所有引用计数相关代码
确保所有类型实现都使用纯 GC 方式
更新文档：
明确说明系统现在使用纯 GC 内存管理
更新开发指南，说明如何正确管理对象引用
优化 GC 性能：
现在可以专注于优化 GC 算法
考虑增加增量 GC 或并发 GC 功能
添加新功能：
实现我们讨论的 delete() API
考虑添加弱引用支持
可能的话添加终结器（finalizer）机制
*/
    //xc_val (*alloc)(int type, ...);//TODO link gc_alloc

    xc_val (*new)(int type, ...);//TODO add flags for "const"
   //xc_val (*delete)(xc_val);//xc_delete

    int (*type_of)(xc_val obj); //"typeof"是c 语言保留字不能用
    int (*is)(xc_val obj, int type);

    int (*register_type)(const char* name, xc_type_lifecycle_t* lifecycle);
    int (*get_type_id)(const char* name);
    //TODO 可以考虑加一个 get_type_by_id()=>xc_type_lifecycle_t* 
    
    //runtime, calling stack
    char (*register_method)(int type, const char* func_name, xc_method_func native_func);
    xc_val (*call)(xc_val obj, const char* method, ...);
    xc_val (*dot)(xc_val obj, const char* key, ...);
    xc_val (*invoke)(xc_val func, int argc, ...);
    
    //exception handling
    xc_val (*try_catch_finally)(xc_val try_func, xc_val catch_func, xc_val finally_func);
    void (*throw)(xc_val error);
    void (*throw_with_rethrow)(xc_val error);
    void (*set_uncaught_exception_handler)(xc_val handler);
    xc_val (*get_current_error)(void);
    void (*clear_error)(void);
} xc_runtime_t;

/* xc global instance */
extern xc_runtime_t xc;

#endif /* XC_H */
/**
PLAN
对象生命周期
创建（内存分配、类型初始化）
引用管理（引用计数增减）
销毁（释放资源、内存回收）
内存管理特殊场景
永久对象（语言常量、单例）
根对象管理（不被 GC 扫描到但需要保留的对象）
弱引用处理（避免引用循环）
GC 控制和优化
强制 GC
暂停/恢复 GC
内存压力监控和响应
VM 内部协作
类型系统与 GC 协作
对象遍历（对象图）
线程安全和并发控制
接口设计原则：
完整而最小：提供足够完整的接口集，但不过度暴露内部细节
分层设计：区分通用接口、特殊用途接口和内部接口
职责明确：每个接口有明确的单一职责
扩展性：设计能适应未来可能的需求变化

xc_val (*alloc)(size_t size, int type_id);    // 基础内存分配
void (*free)(xc_val obj);                      // 基础内存释放
void (*add_ref)(xc_val obj);                   // 增加引用计数
void (*release)(xc_val obj);                   // 减少引用计数
int (*get_ref_count)(xc_val obj);              // 获取当前引用计数
void (*mark_permanent)(xc_val obj);            // 标记永久对象
void (*add_root)(xc_val *obj_ptr);             // 添加根对象
void (*remove_root)(xc_val *obj_ptr);          // 移除根对象
void (*mark_weak_ref)(xc_val ref);             // 标记弱引用
void (*gc_run)(void);                          // 触发 GC
void (*gc_enable)(void);                       // 启用 GC
void (*gc_disable)(void);                      // 禁用 GC
bool (*gc_is_enabled)(void);                   // 检查 GC 状态
 */