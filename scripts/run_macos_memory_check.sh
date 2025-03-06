#!/bin/bash
# XC项目 - macOS内存泄漏检测脚本

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # 无颜色

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT_DIR="$PROJECT_ROOT/tmp/reports"

# 创建报告目录
mkdir -p "$REPORT_DIR"

# 显示帮助信息
show_help() {
    echo "用法: $0 <可执行文件路径> [参数...]"
    echo
    echo "在macOS上运行内存泄漏检测"
    echo
    echo "选项:"
    echo "  -h, --help    显示此帮助信息"
    echo
    echo "示例:"
    echo "  $0 bin/test_array.exe"
    echo "  $0 bin/test_xc.exe"
}

# 检查参数
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    show_help
    exit 0
fi

if [ $# -lt 1 ]; then
    echo -e "${RED}错误: 缺少可执行文件路径${NC}"
    show_help
    exit 1
fi

EXECUTABLE="$1"
shift  # 移除第一个参数，剩余的是传递给可执行文件的参数

# 检查可执行文件是否存在
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}错误: 可执行文件不存在: $EXECUTABLE${NC}"
    exit 1
fi

# 检查是否在macOS上运行
if [ "$(uname)" != "Darwin" ]; then
    echo -e "${RED}错误: 此脚本仅适用于macOS系统${NC}"
    exit 1
fi

echo -e "${BLUE}XC项目 - macOS内存泄漏检测${NC}"
echo "=================================="
echo -e "${YELLOW}检测可执行文件: $EXECUTABLE${NC}"

# 生成报告文件名
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="$REPORT_DIR/memory_report_${TIMESTAMP}.txt"

echo -e "${YELLOW}运行程序并检测内存泄漏...${NC}"
echo -e "${YELLOW}报告将保存到: $REPORT_FILE${NC}"

# 运行程序并使用leaks命令检测内存泄漏
# 注意：leaks命令需要在程序运行时执行，所以我们使用MallocStackLogging环境变量
export MallocStackLogging=1
"$EXECUTABLE" "$@" &
PID=$!

# 等待程序启动
sleep 1

# 运行leaks命令
leaks --list --groupByType "$PID" > "$REPORT_FILE"

# 等待程序结束
wait $PID
EXIT_CODE=$?

# 分析报告
if grep -q "leaks Report Version" "$REPORT_FILE"; then
    LEAK_COUNT=$(grep -c "Leak: " "$REPORT_FILE" || true)
    
    if [ "$LEAK_COUNT" -gt 0 ]; then
        echo -e "${RED}检测到 $LEAK_COUNT 处内存泄漏!${NC}"
        echo -e "${YELLOW}详细信息请查看报告文件: $REPORT_FILE${NC}"
    else
        echo -e "${GREEN}未检测到内存泄漏!${NC}"
    fi
else
    echo -e "${RED}无法生成内存泄漏报告${NC}"
fi

# 返回原程序的退出码
exit $EXIT_CODE 