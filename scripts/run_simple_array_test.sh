#!/bin/bash
# XC项目 - 简化数组类型测试脚本

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # 无颜色

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="$PROJECT_ROOT/bin"
TEST_DIR="$PROJECT_ROOT/test"
LIB_DIR="$PROJECT_ROOT/lib"

# 创建bin目录（如果不存在）
mkdir -p "$BIN_DIR"

echo -e "${BLUE}XC项目 - 简化数组类型测试${NC}"
echo "==============================="

# 编译测试
echo -e "${YELLOW}编译简化数组测试...${NC}"

# 检查测试文件是否存在
if [ ! -f "$TEST_DIR/test_array_simple.c" ]; then
    echo -e "${RED}错误: 测试文件不存在: $TEST_DIR/test_array_simple.c${NC}"
    exit 1
fi

# 检查库文件是否存在
if [ ! -f "$LIB_DIR/libxc.a" ]; then
    echo -e "${YELLOW}警告: 库文件不存在，尝试构建...${NC}"
    make libxc
fi

# 编译命令
gcc -o "$BIN_DIR/test_array_simple.exe" \
    "$TEST_DIR/test_array_simple.c" \
    -I"$PROJECT_ROOT" \
    -L"$LIB_DIR" \
    -lxc

# 检查编译结果
if [ $? -eq 0 ]; then
    echo -e "${GREEN}简化数组测试编译成功!${NC}"
else
    echo -e "${RED}简化数组测试编译失败!${NC}"
    exit 1
fi

# 运行测试
echo -e "${YELLOW}运行简化数组测试...${NC}"

# 检查测试可执行文件是否存在
if [ ! -f "$BIN_DIR/test_array_simple.exe" ]; then
    echo -e "${RED}错误: 测试可执行文件不存在: $BIN_DIR/test_array_simple.exe${NC}"
    exit 1
fi

# 运行测试
"$BIN_DIR/test_array_simple.exe"

# 检查测试结果
if [ $? -eq 0 ]; then
    echo -e "${GREEN}简化数组测试全部通过!${NC}"
else
    echo -e "${RED}简化数组测试失败!${NC}"
    exit 1
fi

echo -e "${GREEN}简化数组测试完成!${NC}" 