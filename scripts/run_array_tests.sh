#!/bin/bash
# XC项目 - 数组类型测试脚本

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
SRC_DIR="$PROJECT_ROOT/src"
INCLUDE_DIR="$PROJECT_ROOT/include"

# 创建bin目录（如果不存在）
mkdir -p "$BIN_DIR"

echo -e "${BLUE}XC项目 - 数组类型测试${NC}"
echo "==============================="

# 编译数组测试
compile_array_test() {
    echo -e "${YELLOW}编译数组类型测试...${NC}"
    
    # 检查测试文件是否存在
    if [ ! -f "$TEST_DIR/test_xc_array.c" ]; then
        echo -e "${RED}错误: 数组测试文件不存在: $TEST_DIR/test_xc_array.c${NC}"
        exit 1
    fi
    
    # 检查兼容性头文件是否存在
    if [ ! -f "$INCLUDE_DIR/xc_compat.h" ]; then
        echo -e "${RED}错误: 兼容性头文件不存在: $INCLUDE_DIR/xc_compat.h${NC}"
        exit 1
    fi
    
    # 编译命令
    gcc -DTEST_ARRAY_STANDALONE \
        -o "$BIN_DIR/test_array.exe" \
        "$TEST_DIR/test_xc_array.c" \
        "$TEST_DIR/test_utils.c" \
        "$SRC_DIR/xc/"*.c \
        "$SRC_DIR/xc/xc_types/"*.c \
        -I"$PROJECT_ROOT" \
        -I"$SRC_DIR" \
        -I"$INCLUDE_DIR"
    
    # 检查编译结果
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}数组测试编译成功!${NC}"
    else
        echo -e "${RED}数组测试编译失败!${NC}"
        exit 1
    fi
}

# 运行数组测试
run_array_test() {
    echo -e "${YELLOW}运行数组类型测试...${NC}"
    
    # 检查测试可执行文件是否存在
    if [ ! -f "$BIN_DIR/test_array.exe" ]; then
        echo -e "${RED}错误: 测试可执行文件不存在: $BIN_DIR/test_array.exe${NC}"
        exit 1
    fi
    
    # 运行测试
    "$BIN_DIR/test_array.exe"
    
    # 检查测试结果
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}数组测试全部通过!${NC}"
    else
        echo -e "${RED}数组测试失败!${NC}"
        exit 1
    fi
}

# 运行带内存检查的测试
run_memory_check() {
    echo -e "${YELLOW}运行带内存检查的数组测试...${NC}"
    
    # 检测操作系统类型
    OS_TYPE=$(uname -s)
    
    if [ "$OS_TYPE" = "Darwin" ]; then
        # macOS平台使用leaks命令
        if [ ! -f "$PROJECT_ROOT/scripts/run_macos_memory_check.sh" ]; then
            echo -e "${RED}错误: macOS内存检查脚本不存在: $PROJECT_ROOT/scripts/run_macos_memory_check.sh${NC}"
            exit 1
        fi
        
        # 运行macOS内存检查
        "$PROJECT_ROOT/scripts/run_macos_memory_check.sh" "$BIN_DIR/test_array.exe"
    else
        # Linux平台使用Valgrind
        if [ ! -f "$PROJECT_ROOT/scripts/run_memory_check.sh" ]; then
            echo -e "${RED}错误: 内存检查脚本不存在: $PROJECT_ROOT/scripts/run_memory_check.sh${NC}"
            exit 1
        fi
        
        # 运行内存检查
        "$PROJECT_ROOT/scripts/run_memory_check.sh" "$BIN_DIR/test_array.exe"
    fi
}

# 主函数
main() {
    # 编译测试
    compile_array_test
    
    # 运行测试
    run_array_test
    
    # 询问是否运行内存检查
    echo
    read -p "是否运行内存检查? (y/n): " run_check
    if [[ "$run_check" =~ ^[Yy]$ ]]; then
        run_memory_check
    fi
    
    echo -e "${GREEN}数组类型测试完成!${NC}"
}

# 执行主函数
main 