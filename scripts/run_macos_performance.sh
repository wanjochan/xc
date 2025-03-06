#!/bin/bash
# XC项目 - macOS性能分析脚本

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
    echo "用法: $0 [选项] <可执行文件路径> [参数...]"
    echo
    echo "在macOS上运行性能分析"
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

# 检查是否在macOS上运行
if [ "$(uname)" != "Darwin" ]; then
    echo -e "${RED}错误: 此脚本仅适用于macOS系统${NC}"
    exit 1
fi

echo -e "${BLUE}XC项目 - macOS性能分析${NC}"
echo "=============================="
echo -e "${YELLOW}分析可执行文件: $EXECUTABLE${NC}"

# 运行内存分析
run_memory_analysis() {
    echo -e "${BLUE}运行内存分析...${NC}"
    
    # 生成报告文件名
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    REPORT_FILE="$REPORT_DIR/memory_report_${TIMESTAMP}.txt"
    
    echo -e "${YELLOW}运行程序并检测内存泄漏...${NC}"
    echo -e "${YELLOW}报告将保存到: $REPORT_FILE${NC}"
    
    # 运行程序并使用leaks命令检测内存泄漏
    export MallocStackLogging=1
    "$EXECUTABLE" "$@" &
    PID=$!
    
    # 等待程序启动
    sleep 1
    
    # 运行leaks命令
    leaks --list --groupByType "$PID" > "$REPORT_FILE"
    
    # 等待程序结束
    wait $PID
    
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
}

# 运行CPU分析
run_cpu_analysis() {
    echo -e "${BLUE}运行CPU分析...${NC}"
    
    # 检查是否安装了Instruments
    if ! command -v xcrun &> /dev/null; then
        echo -e "${RED}错误: 未找到xcrun命令，请确保已安装Xcode命令行工具${NC}"
        echo -e "${YELLOW}可以通过运行 'xcode-select --install' 来安装${NC}"
        return 1
    fi
    
    # 生成报告文件名
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    TRACE_FILE="$REPORT_DIR/cpu_profile_${TIMESTAMP}.trace"
    
    echo -e "${YELLOW}运行程序并分析CPU性能...${NC}"
    echo -e "${YELLOW}跟踪文件将保存到: $TRACE_FILE${NC}"
    
    # 使用Instruments的Time Profiler模板运行程序
    xcrun xctrace record --template 'Time Profiler' --output "$TRACE_FILE" --launch -- "$EXECUTABLE" "$@"
    
    # 检查是否成功生成跟踪文件
    if [ -d "$TRACE_FILE" ]; then
        echo -e "${GREEN}CPU性能分析完成!${NC}"
        echo -e "${YELLOW}跟踪文件已保存到: $TRACE_FILE${NC}"
        echo -e "${YELLOW}要查看详细的性能分析结果，请在Xcode中打开跟踪文件:${NC}"
        echo -e "${YELLOW}open \"$TRACE_FILE\"${NC}"
    else
        echo -e "${RED}CPU性能分析失败!${NC}"
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