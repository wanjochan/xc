#!/bin/bash

# github_release.sh - 自动在GitHub上发布libxc.a和libxc.h
# 使用方法: ./github_release.sh [版本号] [发布说明]
# 例如: ./github_release.sh v1.0.0 "首次正式发布"

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录的上级目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 设置目录
LIB_DIR="${PROJECT_ROOT}/lib"
INCLUDE_DIR="${PROJECT_ROOT}/include"
RELEASE_DIR="${PROJECT_ROOT}/release"

# 检查参数
if [ $# -lt 1 ]; then
    echo "错误: 缺少版本号参数"
    echo "使用方法: $0 [版本号] [发布说明]"
    echo "例如: $0 v1.0.0 \"首次正式发布\""
    exit 1
fi

VERSION="$1"
RELEASE_NOTES="${2:-"libxc ${VERSION} 发布"}"
RELEASE_TAG="libxc-${VERSION}"
RELEASE_NAME="libxc ${VERSION}"
RELEASE_PACKAGE="libxc-${VERSION}"

echo "准备发布 libxc ${VERSION}..."

# 确保构建最新版本
echo "构建最新版本的libxc..."
"${SCRIPT_DIR}/build_libxc.sh"

# 检查必要文件是否存在
if [ ! -f "${LIB_DIR}/libxc.a" ]; then
    echo "错误: ${LIB_DIR}/libxc.a 不存在"
    exit 1
fi

if [ ! -f "${INCLUDE_DIR}/libxc.h" ]; then
    echo "错误: ${INCLUDE_DIR}/libxc.h 不存在"
    exit 1
fi

# 创建发布目录
mkdir -p "${RELEASE_DIR}/${RELEASE_PACKAGE}"
mkdir -p "${RELEASE_DIR}/${RELEASE_PACKAGE}/lib"
mkdir -p "${RELEASE_DIR}/${RELEASE_PACKAGE}/include"

# 复制文件到发布目录
echo "准备发布文件..."
cp "${LIB_DIR}/libxc.a" "${RELEASE_DIR}/${RELEASE_PACKAGE}/lib/"
cp "${INCLUDE_DIR}/libxc.h" "${RELEASE_DIR}/${RELEASE_PACKAGE}/include/"

# 创建README文件
cat > "${RELEASE_DIR}/${RELEASE_PACKAGE}/README.md" << EOF
# libxc ${VERSION}

XC是一个轻量级运行时/语言实现，具有类似JavaScript的类型系统和内存管理机制。

## 文件说明

- \`lib/libxc.a\`: XC运行时库静态链接库
- \`include/libxc.h\`: 完整的预处理头文件（推荐使用）

## 使用方法

### 简单集成（推荐）

只需包含完整的预处理头文件：

\`\`\`c
#include "libxc.h"
\`\`\`

### 模块化集成

可以选择只包含需要的模块：

\`\`\`c
#include "xc.h"           // 核心运行时
#include "xc_types.h"     // 类型系统
#include "xc_gc.h"        // 垃圾回收
// ...其他需要的模块
\`\`\`

## 链接

编译时链接 \`libxc.a\`：

\`\`\`bash
gcc -o myapp myapp.c -I./include -L./lib -lxc
\`\`\`

## 版本说明

${RELEASE_NOTES}
EOF

# 创建发布包
echo "创建发布包..."
cd "${RELEASE_DIR}"
tar -czf "${RELEASE_PACKAGE}.tar.gz" "${RELEASE_PACKAGE}"
zip -r "${RELEASE_PACKAGE}.zip" "${RELEASE_PACKAGE}"

echo "发布包已创建:"
ls -la "${RELEASE_DIR}/${RELEASE_PACKAGE}.tar.gz"
ls -la "${RELEASE_DIR}/${RELEASE_PACKAGE}.zip"

# 使用GitHub CLI创建发布（如果安装了gh命令）
if command -v gh &> /dev/null; then
    echo "使用GitHub CLI创建发布..."
    
    # 创建发布
    gh release create "${RELEASE_TAG}" \
        --title "${RELEASE_NAME}" \
        --notes "${RELEASE_NOTES}" \
        "${RELEASE_DIR}/${RELEASE_PACKAGE}.tar.gz" \
        "${RELEASE_DIR}/${RELEASE_PACKAGE}.zip"
    
    echo "GitHub发布已创建: ${RELEASE_TAG}"
else
    echo "未找到GitHub CLI (gh)。请手动创建发布:"
    echo "1. 访问GitHub仓库的Releases页面"
    echo "2. 点击'Draft a new release'"
    echo "3. 标签: ${RELEASE_TAG}"
    echo "4. 标题: ${RELEASE_NAME}"
    echo "5. 描述: ${RELEASE_NOTES}"
    echo "6. 上传文件: ${RELEASE_DIR}/${RELEASE_PACKAGE}.tar.gz 和 ${RELEASE_DIR}/${RELEASE_PACKAGE}.zip"
fi

echo "发布过程完成!"