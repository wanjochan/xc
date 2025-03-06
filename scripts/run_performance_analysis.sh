#!/bin/bash
# XC项目 - 性能分析脚本
# 此脚本会根据操作系统选择合适的性能分析工具

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # 无颜色

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"
REPORT_DIR="$PROJECT_ROOT/tmp/reports"

# 创建报告目录
mkdir -p "$REPORT_DIR"

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项] <可执行文件路径> [参数...]"
    echo
    echo "根据操作系统选择合适的性能分析工具运行性能分析"
    echo
    echo "选项:"
    echo "  -h, --help    显示此帮助信息"
    echo "  -m, --memory  运行内存分析"
    echo "  -c, --cpu     运行CPU分析"
    echo "  -a, --all     运行所有分析（默认）"
    echo
    echo "示例:"
    echo "  $0 bin/test_array.exe"
    echo "  $0 -m bin/test_xc.exe"
    echo "  $0 -c bin/test_xc.exe"
}

# 默认分析类型
RUN_MEMORY=0
RUN_CPU=0

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -m|--memory)
            RUN_MEMORY=1
            shift
            ;;
        -c|--cpu)
            RUN_CPU=1
            shift
            ;;
        -a|--all)
            RUN_MEMORY=1
            RUN_CPU=1
            shift
            ;;
        *)
            break
            ;;
    esac
done

# 如果没有指定分析类型，则运行所有分析
if [ $RUN_MEMORY -eq 0 ] && [ $RUN_CPU -eq 0 ]; then
    RUN_MEMORY=1
    RUN_CPU=1
fi

# 检查参数
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

echo -e "${BLUE}XC项目 - 性能分析${NC}"
echo "========================="
echo -e "${YELLOW}分析可执行文件: $EXECUTABLE${NC}"

# 检测操作系统类型
OS_TYPE=$(uname -s)
echo -e "${YELLOW}检测到操作系统: $OS_TYPE${NC}"

# 运行内存分析
run_memory_analysis() {
    echo -e "${BLUE}运行内存分析...${NC}"
    
    if [ "$OS_TYPE" = "Darwin" ]; then
        # macOS平台使用leaks命令
        if [ ! -f "$SCRIPTS_DIR/run_macos_memory_check.sh" ]; then
            echo -e "${RED}错误: macOS内存检查脚本不存在: $SCRIPTS_DIR/run_macos_memory_check.sh${NC}"
            return 1
        fi
        
        # 运行macOS内存检查
        "$SCRIPTS_DIR/run_macos_memory_check.sh" "$EXECUTABLE" "$@"
    else
        # Linux平台使用Valgrind
        if [ ! -f "$SCRIPTS_DIR/run_memory_check.sh" ]; then
            echo -e "${RED}错误: 内存检查脚本不存在: $SCRIPTS_DIR/run_memory_check.sh${NC}"
            return 1
        fi
        
        # 运行内存检查
        "$SCRIPTS_DIR/run_memory_check.sh" "$EXECUTABLE" "$@"
    fi
}

# 运行CPU分析
run_cpu_analysis() {
    echo -e "${BLUE}运行CPU分析...${NC}"
    
    if [ "$OS_TYPE" = "Darwin" ]; then
        # macOS平台使用Instruments
        if [ ! -f "$SCRIPTS_DIR/run_macos_cpu_profile.sh" ]; then
            echo -e "${RED}错误: macOS CPU分析脚本不存在: $SCRIPTS_DIR/run_macos_cpu_profile.sh${NC}"
            return 1
        fi
        
        # 运行macOS CPU分析
        "$SCRIPTS_DIR/run_macos_cpu_profile.sh" "$EXECUTABLE" "$@"
    else
        # Linux平台使用Valgrind/Callgrind
        if [ ! -f "$SCRIPTS_DIR/run_cpu_profile.sh" ]; then
            echo -e "${RED}错误: CPU分析脚本不存在: $SCRIPTS_DIR/run_cpu_profile.sh${NC}"
            return 1
        fi
        
        # 运行CPU分析
        "$SCRIPTS_DIR/run_cpu_profile.sh" "$EXECUTABLE" "$@"
    fi
}

# 运行性能分析
if [ $RUN_MEMORY -eq 1 ]; then
    run_memory_analysis "$@"
fi

if [ $RUN_CPU -eq 1 ]; then
    run_cpu_analysis "$@"
fi

echo -e "${GREEN}性能分析完成!${NC}" 