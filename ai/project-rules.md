# 当前项目的规则

## 基本规则

- 与用户对话或自己思考要用中文，保持简洁（concise）和准确（precise）
- 代码中的注释必须使用英文，而且太明显的不用加注释！
- 进入工作后，不要习惯性马上修改文件，要从概念数量、步骤繁琐度、逻辑嵌套深度、领域知识依赖度评估复杂度进行简洁且准确的推理思考（把思考内容用```...```代码块包含起来）；

## 项目规则

- 我们的工作流程使用的是 tasker-v2.9.md，每轮会话开始前要认真阅读该工作流文档
- 打造一个 c 语言轻量运行库 libxc，设计文档为 xc.md
- 非必要情况下减少宏的使用
- 深刻理解核心结构 cosmopolitan=>infrax层=>xc运行时=>libxc.a的层次结构，
  - infrax 层代码不要直接引用 libc
  - xc 运行时只能引用 infra层，不要直接引用 libc 层
  - 对外交付的是 libxc.a 和 libxc.h，其它头文件放在 src/internal/中
- 编译与测试：
```

编译 libxc：
sh scripts/build_libxc.sh

基准测试：
timeout 20s sh scripts/build_text_xc.sh

其它基本测试命令
- `cd ~/xc && make libxc`: 构建libxc.a静态库
- `cd ~/xc && make test`: 运行测试程序验证功能
- `cd ~/xc && ls -la lib/libxc.a`: 检查静态库是否生成
- `cd ~/xc && ls -la include/libxc.h`: 检查头文件是否生成
```
    
