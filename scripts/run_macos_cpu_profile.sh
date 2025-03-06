#!/bin/bash
# XC项目 - macOS CPU性能分析脚本

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
    echo "在macOS上运行CPU性能分析"
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

# 检查是否安装了Instruments
if ! command -v xcrun &> /dev/null; then
    echo -e "${RED}错误: 未找到xcrun命令，请确保已安装Xcode命令行工具${NC}"
    echo -e "${YELLOW}可以通过运行 'xcode-select --install' 来安装${NC}"
    exit 1
fi

echo -e "${BLUE}XC项目 - macOS CPU性能分析${NC}"
echo "================================"
echo -e "${YELLOW}分析可执行文件: $EXECUTABLE${NC}"

# 生成报告文件名
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
TRACE_FILE="$REPORT_DIR/cpu_profile_${TIMESTAMP}.trace"
REPORT_FILE="$REPORT_DIR/cpu_profile_${TIMESTAMP}.txt"

echo -e "${YELLOW}运行程序并分析CPU性能...${NC}"
echo -e "${YELLOW}跟踪文件将保存到: $TRACE_FILE${NC}"
echo -e "${YELLOW}报告将保存到: $REPORT_FILE${NC}"

# 使用Instruments的Time Profiler模板运行程序
xcrun xctrace record --template 'Time Profiler' --output "$TRACE_FILE" --launch -- "$EXECUTABLE" "$@"

# 检查是否成功生成跟踪文件
if [ -f "$TRACE_FILE" ]; then
    echo -e "${GREEN}CPU性能分析完成!${NC}"
    echo -e "${YELLOW}跟踪文件已保存到: $TRACE_FILE${NC}"
    
    # 提取性能数据（这里使用简单的方法，实际上需要更复杂的处理）
    echo "CPU性能分析报告" > "$REPORT_FILE"
    echo "===================" >> "$REPORT_FILE"
    echo "生成时间: $(date)" >> "$REPORT_FILE"
    echo "分析文件: $EXECUTABLE" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "注意: 要查看详细的性能分析结果，请在Xcode中打开跟踪文件:" >> "$REPORT_FILE"
    echo "$TRACE_FILE" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "使用以下命令打开:" >> "$REPORT_FILE"
    echo "open \"$TRACE_FILE\"" >> "$REPORT_FILE"
    
    echo -e "${YELLOW}要查看详细的性能分析结果，请在Xcode中打开跟踪文件:${NC}"
    echo -e "${YELLOW}open \"$TRACE_FILE\"${NC}"
else
    echo -e "${RED}CPU性能分析失败!${NC}"
    exit 1
fi 