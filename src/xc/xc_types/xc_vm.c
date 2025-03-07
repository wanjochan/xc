#include "../xc.h"
// #include "../xc_gc.h"  // Removed since we've merged it into xc.c
#include "../xc_internal.h"

/**
## design notes

权衡考虑
然而，AST方案也有一些权衡需要考虑：
1. 性能差异
字节码VM: 通常更快，特别是在热点代码路径
AST VM: 遍历开销较大，特别是深层嵌套结构
2. 内存占用
字节码: 更紧凑，尤其对于大型程序
AST: 节点间关系需要存储指针，占用更多内存
3. JIT编译
字节码通常更容易转为机器码
AST需要额外转换步骤
4. 二进制格式
字节码天然适合二进制格式
AST需要序列化/反序列化机制
混合方案
实际上，许多现代VM采用混合方案：
用AST进行初始解析和优化
将AST转换为字节码进行执行
保留AST用于调试信息
对于XC来说，如果追求极简实现而对性能要求不那么严格，纯AST方案会大大简化结构化控制流的实现。若需要更好性能，可以考虑AST→字节码的转换步骤。
这种方式比直接实现所有WASM字节码指令要简单得多，特别是对于复杂的控制流指令和类型检查
 */
/* 字节码指令类型 */
//TODO 考虑，是不是有些指令参考一下 JVM/WASM 的指令集好一点。
typedef enum {
    OP_NOP,        // 空操作
    OP_PUSH_VAL,   // 压入值

//TODO 压入的应该是 胖指针 值类型, 下面这些没用
    // OP_PUSH_NULL,  // 压入 null
    // OP_PUSH_BOOL,  // 压入布尔值
    // OP_PUSH_NUM,   // 压入数字
    // OP_PUSH_STR,   // 压入字符串

    OP_POP,        // 弹出栈顶
    OP_ADD,        // 加法
    OP_SUB,        // 减法
    OP_MUL,        // 乘法
    OP_DIV,        // 除法
    OP_GET,        // 获取属性
    OP_SET,        // 设置属性
    OP_CALL,       // 调用函数
    OP_RET,        // 返回
    OP_JMP,        // 无条件跳转
    OP_JMP_IF,     // 条件跳转
    OP_STORE,      // 存储变量
    OP_LOAD,       // 加载变量
    // ... 其他必要指令
} xc_opcode_t;

/* 字节码指令结构 */
typedef struct {
    xc_opcode_t op;     // 操作码
    union {
        double num;     // 数字常量
        char* str;      // 字符串常量
        int jump;       // 跳转偏移
        int arg_count;  // 参数数量
        int index;      // 变量索引
    } operand;
} xc_instruction_t;

/* VM 类型数据结构 */
typedef struct {
    xc_val* stack;          // 操作数栈
    int stack_size;         // 栈大小
    int stack_top;          // 栈顶指针
    
    xc_instruction_t* code; // 字节码
    int code_size;          // 字节码大小
    int code_capacity;      // 字节码容量
    int pc;                 // 程序计数器
    
    xc_val scope;           // 当前作用域 (使用 object 类型)
    xc_val global;          // 全局作用域
    
    xc_val result;          // 最后执行结果
} xc_vm_t;

/* Forward declarations */
static void vm_marker(xc_val self, void (*mark_func)(xc_val));
static int vm_destroy(xc_val self);
static void vm_initialize(void);
static xc_val vm_creator(int type, va_list args);
static xc_val vm_to_string(xc_val self, xc_val arg);
static xc_val vm_execute(xc_val self, xc_val arg);
static xc_val vm_add_instruction(xc_val self, xc_val arg);
static xc_val vm_reset(xc_val self, xc_val arg);
static xc_val vm_get_result(xc_val self, xc_val arg);
static xc_val vm_set_global(xc_val self, xc_val arg);

/* VM 类型方法 */
static xc_val vm_to_string(xc_val self, xc_val arg) {
    return xc.create(XC_TYPE_STRING, "vm");
}

/* 执行 VM 中的字节码 */
static xc_val vm_execute(xc_val self, xc_val arg) {
    // TODO: 实现字节码执行逻辑
    return xc.create(XC_TYPE_NULL);
}

/* 添加指令到 VM */
static xc_val vm_add_instruction(xc_val self, xc_val arg) {
    // TODO: 实现添加指令逻辑
    return xc.create(XC_TYPE_BOOL, 1);
}

/* 重置 VM 状态 */
static xc_val vm_reset(xc_val self, xc_val arg) {
    // TODO: 实现重置逻辑
    return xc.create(XC_TYPE_BOOL, 1);
}

/* 获取执行结果 */
static xc_val vm_get_result(xc_val self, xc_val arg) {
    // TODO: 实现获取结果逻辑
    return xc.create(XC_TYPE_NULL);
}

/* 设置全局变量 */
static xc_val vm_set_global(xc_val self, xc_val arg) {
    // TODO: 实现设置全局变量逻辑
    return xc.create(XC_TYPE_BOOL, 1);
}

/* VM 类型的 GC 标记方法 */
static void vm_marker(xc_val self, void (*mark_func)(xc_val)) {
    // TODO: 实现标记逻辑
}

/* VM 类型的销毁方法 */
static int vm_destroy(xc_val self) {
    // TODO: 实现销毁逻辑
    return 0;
}

/* VM 类型初始化 */
static void vm_initialize(void) {
    // 注册方法
    xc.register_method(XC_TYPE_VM, "toString", vm_to_string);
    xc.register_method(XC_TYPE_VM, "execute", vm_execute);
    xc.register_method(XC_TYPE_VM, "addInstruction", vm_add_instruction);
    xc.register_method(XC_TYPE_VM, "reset", vm_reset);
    xc.register_method(XC_TYPE_VM, "getResult", vm_get_result);
    xc.register_method(XC_TYPE_VM, "setGlobal", vm_set_global);
}

/* VM 类型创建函数 */
static xc_val vm_creator(int type, va_list args) {
    // TODO: 实现创建逻辑
    return NULL;
}

/* 注册 VM 类型 */
int _xc_type_vm = XC_TYPE_VM;

/* Type descriptor for vm type */
static xc_type_lifecycle_t vm_type = {
    .initializer = vm_initialize,
    .cleaner = NULL,
    .creator = vm_creator,
    .destroyer = vm_destroy,
    .marker = vm_marker,
    // .allocator = NULL,
    .name = "vm",
    .equal = NULL,
    .compare = NULL,
    .flags = 0
};

__attribute__((constructor)) static void register_vm_type(void) {
    /* 注册类型 */
    _xc_type_vm = xc.register_type("vm", &vm_type);
    // vm_initialize();
}

/* 创建 VM 对象 */
xc_val xc_vm_create(int stack_size) {
    return xc.create(XC_TYPE_VM, stack_size);
}

/* 使用示例:

// 创建 VM 对象
xc_val vm = xc_vm_create(256);

// 添加指令
xc_val instr1 = xc.create(XC_TYPE_ARRAY, 2, xc.create(XC_TYPE_NUMBER, OP_PUSH_NUM), xc.create(XC_TYPE_NUMBER, 10));
xc.call(vm, "addInstruction", instr1);

xc_val instr2 = xc.create(XC_TYPE_ARRAY, 2, xc.create(XC_TYPE_NUMBER, OP_PUSH_NUM), xc.create(XC_TYPE_NUMBER, 20));
xc.call(vm, "addInstruction", instr2);

xc_val instr3 = xc.create(XC_TYPE_ARRAY, 1, xc.create(XC_TYPE_NUMBER, OP_ADD));
xc.call(vm, "addInstruction", instr3);

xc_val instr4 = xc.create(XC_TYPE_ARRAY, 1, xc.create(XC_TYPE_NUMBER, OP_RET));
xc.call(vm, "addInstruction", instr4);

// 执行 VM
xc.call(vm, "execute", NULL);

// 获取结果
xc_val result = xc.call(vm, "getResult", NULL);
xc.print(result); // 输出 30
*/ 