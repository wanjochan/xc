# XC 运行时自动垃圾回收机制

## 1. 概述

XC 运行时采用自动垃圾回收（Auto GC）机制来管理内存，使开发者无需手动分配和释放内存。本文档详细描述了 XC 运行时的垃圾回收系统设计、实现和使用方法。

### 1.1 设计目标

- **自动化内存管理**：减轻开发者手动管理内存的负担
- **高效性**：最小化 GC 对程序性能的影响
- **可靠性**：避免内存泄漏和悬挂指针问题
- **可扩展性**：支持各种类型的对象和复杂的对象图

### 1.2 历史演进

XC 运行时的内存管理经历了从引用计数到纯 GC 的演进：

1. **早期版本**：使用引用计数（reference counting）机制
2. **过渡阶段**：混合使用引用计数和标记-清除 GC
3. **当前版本**：采用纯标记-清除（mark-sweep）GC 机制

## 2. 核心概念

### 2.1 对象结构

所有由 GC 管理的对象都有一个通用的头部结构：

```c
typedef struct xc_object {
    size_t size;              /* 对象总大小（字节） */
    int type_id;              /* 类型ID */
    int ref_count;            /* 引用计数（历史遗留，当前版本不使用） */
    int gc_color;             /* GC 标记颜色 */
    struct xc_object *gc_next; /* GC 链表中的下一个对象 */
    /* 对象数据跟在此结构后面 */
} xc_object_t;
```

### 2.2 三色标记算法

XC 运行时使用三色标记算法进行垃圾回收：

- **白色（White）**：可能不可达的对象，是回收的候选者
- **灰色（Gray）**：已知可达但其引用尚未处理的对象
- **黑色（Black）**：已知可达且其所有引用都已处理的对象
- **永久（Permanent）**：特殊标记，表示对象永远不会被回收

### 2.3 对象链表

GC 系统使用三个链表来管理不同颜色的对象：

- **white_list**：白色对象链表
- **gray_list**：灰色对象链表
- **black_list**：黑色对象链表

这些链表通过对象的 `gc_next` 字段连接。

## 3. GC 工作流程

### 3.1 初始化

GC 系统在运行时初始化时创建：

```c
void xc_gc_init(xc_runtime_t *rt, const xc_gc_config_t *config);
```

初始化过程包括：
- 创建 GC 上下文
- 设置初始配置（堆大小、GC 触发阈值等）
- 初始化对象链表和根集合

### 3.2 对象分配

当需要创建新对象时，系统调用：

```c
xc_object_t *xc_gc_alloc(xc_runtime_t *rt, size_t size, int type_id);
```

分配过程：
1. 检查是否需要触发 GC
2. 分配内存
3. 初始化对象头部
4. 将对象添加到 white_list

### 3.3 标记阶段

标记阶段从根对象开始，标记所有可达对象：

```c
void xc_gc_mark_roots(xc_runtime_t *rt);
void xc_gc_process_gray_list(xc_runtime_t *rt);
```

标记过程：
1. 将所有根对象标记为灰色，加入 gray_list
2. 从 gray_list 取出对象，处理其引用
3. 将引用的对象标记为灰色，加入 gray_list
4. 将处理完的对象标记为黑色，移到 black_list
5. 重复步骤 2-4，直到 gray_list 为空

### 3.4 清除阶段

清除阶段回收所有不可达对象：

```c
size_t xc_gc_sweep(xc_runtime_t *rt);
```

清除过程：
1. 遍历 white_list 中的所有对象
2. 对每个对象调用其类型的 destroyer 函数
3. 释放对象内存
4. 返回回收的内存总量

### 3.5 完整 GC 周期

一个完整的 GC 周期由以下步骤组成：

```c
void xc_gc_run(xc_runtime_t *rt);
```

1. 重置所有对象颜色（将黑色和灰色对象重置为白色）
2. 标记阶段：标记所有可达对象
3. 清除阶段：回收所有不可达对象
4. 更新统计信息

## 4. 类型系统与 GC 的交互

### 4.1 类型生命周期接口

每种类型都需要实现一个生命周期接口，其中包含 GC 相关的回调函数：

```c
typedef struct {
    // ...其他字段...
    xc_marker_func marker;    /* GC 标记函数 */
    xc_destroy_func destroyer; /* 对象销毁函数 */
    // ...其他字段...
} xc_type_lifecycle_t;
```

### 4.2 标记函数

标记函数负责告诉 GC 系统对象引用了哪些其他对象：

```c
typedef void (*xc_marker_func)(xc_val obj, void (*mark_func)(xc_val));
```

正确的实现应该遍历对象的所有引用，并对每个引用调用提供的 mark_func 回调：

```c
static void array_mark(xc_val obj, void (*mark_func)(xc_val)) {
    xc_array_t *arr = (xc_array_t *)obj;
    for (size_t i = 0; i < arr->length; i++) {
        if (arr->items[i]) {
            mark_func(arr->items[i]);
        }
    }
}
```

### 4.3 销毁函数

销毁函数负责释放对象持有的资源（不包括对象本身的内存）：

```c
typedef int (*xc_destroy_func)(xc_val obj);
```

例如，释放文件句柄、网络连接等非内存资源。

## 5. 根对象管理

### 5.1 什么是根对象

根对象是 GC 标记阶段的起点，包括：
- 全局变量
- 栈上的局部变量
- 寄存器中的引用
- 显式注册的根对象

### 5.2 根对象 API

XC 运行时提供了管理根对象的 API：

```c
void xc_gc_add_root(xc_runtime_t *rt, xc_object_t **root_ptr);
void xc_gc_remove_root(xc_runtime_t *rt, xc_object_t **root_ptr);
```

这些函数允许显式管理对象的生命周期，确保特定对象不会被 GC 回收。

### 5.3 永久对象

某些对象（如语言内置的单例对象）需要永远存在，可以标记为永久对象：

```c
void xc_gc_mark_permanent(xc_runtime_t *rt, xc_object_t *obj);
```

永久对象不会被 GC 回收，即使没有引用指向它们。

## 6. GC 控制

### 6.1 启用/禁用 GC

可以临时禁用 GC，例如在性能关键的代码段：

```c
void xc_gc_enable(xc_runtime_t *rt);
void xc_gc_disable(xc_runtime_t *rt);
bool xc_gc_is_enabled(xc_runtime_t *rt);
```

### 6.2 强制 GC

可以显式触发 GC 运行：

```c
void xc_gc_run(xc_runtime_t *rt);
```

### 6.3 GC 统计

可以获取 GC 的运行统计信息：

```c
xc_gc_stats_t xc_gc_get_stats(xc_runtime_t *rt);
void xc_gc_print_stats(xc_runtime_t *rt);
```

## 7. 最佳实践

### 7.1 对象引用管理

- **正确实现 marker 函数**：确保所有引用都被正确标记
- **避免直接操作 GC 内部字段**：不要直接修改 gc_color 或 gc_next
- **使用公共 API**：通过 XC 运行时提供的 API 管理对象

### 7.2 性能优化

- **减少临时对象**：尽量重用对象，减少 GC 压力
- **批量操作**：将多个小操作合并为一个大操作
- **适当使用 GC 控制**：在性能关键的代码段临时禁用 GC

### 7.3 资源管理

对于非内存资源（如文件句柄、网络连接等），有几种管理方式：

1. **在 destroyer 函数中释放**：当对象被 GC 回收时自动释放资源
2. **提供显式关闭方法**：允许用户主动释放资源
3. **使用资源包装模式**：确保资源在特定作用域结束时释放

## 8. 常见问题与解决方案

### 8.1 内存泄漏

可能的原因：
- 对象仍被引用，但实际上不再需要
- marker 函数实现不正确，未标记所有引用
- 对象被错误地添加为根对象但未移除

解决方案：
- 检查对象引用关系
- 确保正确实现 marker 函数
- 检查根对象管理

### 8.2 过早回收

可能的原因：
- 对象引用未被正确标记
- 根对象管理不当

解决方案：
- 确保所有引用都被正确标记
- 适当使用根对象 API 或永久对象标记

### 8.3 性能问题

可能的原因：
- GC 运行过于频繁
- 大量临时对象创建
- 对象图过于复杂

解决方案：
- 调整 GC 触发阈值
- 减少临时对象创建
- 优化对象结构，减少引用层级

## 9. 未来发展方向

XC 运行时的 GC 系统可能的改进方向：

- **增量 GC**：将 GC 工作分散到多个小步骤，减少暂停时间
- **并发 GC**：在后台线程中运行 GC，减少对主线程的影响
- **分代 GC**：根据对象年龄分类处理，提高效率
- **精确 GC**：更准确地识别指针和非指针数据
- **弱引用支持**：添加弱引用机制，解决特定场景下的内存管理问题
- **终结器机制**：为对象提供被回收前的清理回调

## 10. 参考资料

- [The Garbage Collection Handbook](https://gchandbook.org/)
- [Memory Management in V8](https://v8.dev/blog/trash-talk)
- [Java Garbage Collection Basics](https://www.oracle.com/webfolder/technetwork/tutorials/obe/java/gc01/index.html)
- [Python's Garbage Collector](https://devguide.python.org/internals/garbage-collector/)
