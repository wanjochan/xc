# XC - 极致轻量的C运行时引擎

XC是一个非常小的中层次C运行时库，具有类型系统和自动垃圾回收等关键特性。

>- Q: 为何造轮子？
>- A: 因为在开发大数据相关工具时我发现现有的底层库不达我的想法，既然有了自动开发流程[ai-coding](https://github.com/wanjochan/ai-coding)，何不让 AI全自动开发一套？
>- Q: 目前这个库多少代码是 AI 写的？
>- A: 超过 95%
>- Q: 这个库生产备可吗？
>- A: 不知道，反正是 AI 自己写，我不关心

## 项目结构

```
/Users/wjc/xc/
├── src/              # 源代码目录
│   ├── xc/           # XC运行时核心代码
│   └── infrax/       # 基础设施层代码
├── include/          # 公共头文件
├── lib/              # 编译生成的库文件
├── bin/              # 编译生成的可执行文件
├── test/             # 测试程序
├── scripts/          # 构建脚本
│   ├── build_libxc.sh   # 构建libxc.a的脚本
│   └── build_test_xc.sh # 构建和运行测试的脚本
├── docs/             # 文档
└── ai/               # AI辅助任务
    └── tasks/        # AI任务定义
```

## 开发人员

人工智能助理（vscode+roo、cursor） + 流程（ai-coding）

## 构建系统

本项目使用Makefile和自定义构建脚本管理构建过程。默认使用Cosmopolitan提供的`cosmocc`编译器。
在缺少Cosmopolitan工具链时，可通过环境变量`COSMOCC`指定系统编译器（如`gcc`或`cc`），所有脚本都会自动回退到该编译器。

### 主要构建目标

- `make all`: 构建libxc.a和测试程序（默认）
- `make libxc`: 只构建libxc.a静态库
- `make test`: 构建并运行测试程序
- `make clean`: 清理所有构建产物

## 项目层次结构

```
cosmopolitan => infrax层 => xc运行时 => libxc.a + libxc.h
```

1. **cosmopolitan**: 底层基础库，提供跨平台支持
2. **infrax层**: 基础设施层，提供内存管理、线程、网络等基础功能
3. **xc运行时**: 核心运行时，实现类型系统、垃圾回收、异常处理等
4. **libxc.a + libxc.h**: 最终产出的库文件和头文件

## 核心特性

- 类型系统：基本类型定义、类型注册和管理机制
- 内存管理：对象分配和释放、自动垃圾回收（三色标记法）
- 函数系统：函数对象的创建和调用、闭包支持
- 错误处理：完整的try/catch/finally异常处理机制
- 运行栈管理：调用栈跟踪、栈帧管理

## 使用方法

1. 构建libxc.a:
   ```bash
   make libxc
   ```

2. 在你的项目中包含头文件并链接库文件:
   ```c
   #include <libxc.h>
   ```

   编译命令:
   ```bash
   $COSMOCC -I/path/to/xc/include your_file.c -L/path/to/xc/lib -lxc -o your_program
   ```
