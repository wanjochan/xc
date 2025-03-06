#!/bin/bash
# XC项目 - Valgrind安装和配置脚本
# 此脚本用于安装Valgrind性能分析工具并配置用于XC项目的分析

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
CONFIG_DIR="$PROJECT_ROOT/config"
VALGRIND_SUPPRESSION_FILE="$CONFIG_DIR/xc_valgrind.supp"

# 创建配置目录
mkdir -p "$CONFIG_DIR"

echo -e "${BLUE}XC项目 - Valgrind安装和配置${NC}"
echo "========================================"

# 检查操作系统类型
OS_TYPE=$(uname -s)
echo -e "${YELLOW}检测到操作系统: $OS_TYPE${NC}"

# 安装Valgrind
install_valgrind() {
    echo -e "${YELLOW}正在安装Valgrind...${NC}"
    
    case "$OS_TYPE" in
        "Darwin")
            if command -v brew &> /dev/null; then
                echo "使用Homebrew安装Valgrind..."
                brew install valgrind
            else
                echo -e "${RED}错误: 未找到Homebrew。在macOS上，请先安装Homebrew: https://brew.sh/${NC}"
                exit 1
            fi
            ;;
        "Linux")
            if command -v apt-get &> /dev/null; then
                echo "使用apt安装Valgrind..."
                sudo apt-get update
                sudo apt-get install -y valgrind
            elif command -v yum &> /dev/null; then
                echo "使用yum安装Valgrind..."
                sudo yum install -y valgrind
            elif command -v dnf &> /dev/null; then
                echo "使用dnf安装Valgrind..."
                sudo dnf install -y valgrind
            else
                echo -e "${RED}错误: 未找到支持的包管理器。请手动安装Valgrind。${NC}"
                exit 1
            fi
            ;;
        *)
            echo -e "${RED}错误: 不支持的操作系统: $OS_TYPE${NC}"
            exit 1
            ;;
    esac
    
    echo -e "${GREEN}Valgrind安装完成!${NC}"
}

# 验证Valgrind安装
verify_valgrind() {
    echo -e "${YELLOW}验证Valgrind安装...${NC}"
    
    if command -v valgrind &> /dev/null; then
        VALGRIND_VERSION=$(valgrind --version)
        echo -e "${GREEN}Valgrind已成功安装: $VALGRIND_VERSION${NC}"
    else
        echo -e "${RED}错误: Valgrind安装失败或未添加到PATH。${NC}"
        exit 1
    fi
}

# 创建Valgrind抑制文件
create_suppression_file() {
    echo -e "${YELLOW}创建Valgrind抑制文件...${NC}"
    
    cat > "$VALGRIND_SUPPRESSION_FILE" << 'EOF'
# XC项目的Valgrind抑制文件
# 用于抑制已知的误报或不相关的内存问题

# 抑制libc中的已知问题
{
   libc_cond_init_suppress
   Memcheck:Leak
   match-leak-kinds: possible
   fun:calloc
   fun:allocate_dtv
   fun:_dl_allocate_tls
}

# 抑制macOS特定问题
{
   darwin_dyld_suppressions
   Memcheck:Leak
   match-leak-kinds: all
   fun:malloc
   fun:strdup
   fun:_dyld_register_func_for_add_image
}

# 抑制XC测试框架中的已知问题
{
   xc_test_framework_suppressions
   Memcheck:Leak
   match-leak-kinds: definite
   fun:malloc
   fun:xc_test_setup
}
EOF
    
    echo -e "${GREEN}Valgrind抑制文件已创建: $VALGRIND_SUPPRESSION_FILE${NC}"
}

# 创建内存泄漏检测脚本
create_leak_check_script() {
    echo -e "${YELLOW}创建内存泄漏检测脚本...${NC}"
    
    LEAK_CHECK_SCRIPT="$SCRIPTS_DIR/run_memory_check.sh"
    
    cat > "$LEAK_CHECK_SCRIPT" << 'EOF'
#!/bin/bash
# XC项目 - 内存泄漏检测脚本

set -e  # 遇到错误立即退出

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_DIR="$PROJECT_ROOT/config"
VALGRIND_SUPPRESSION_FILE="$CONFIG_DIR/xc_valgrind.supp"
REPORT_DIR="$PROJECT_ROOT/reports"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="$REPORT_DIR/memory_report_$TIMESTAMP.txt"

# 创建报告目录
mkdir -p "$REPORT_DIR"

# 检查参数
if [ $# -lt 1 ]; then
    echo "用法: $0 <可执行文件> [参数...]"
    echo "例如: $0 bin/test_xc.exe"
    exit 1
fi

EXECUTABLE="$1"
shift  # 移除第一个参数，剩余的是传递给可执行文件的参数

# 检查可执行文件是否存在
if [ ! -f "$EXECUTABLE" ]; then
    echo "错误: 可执行文件不存在: $EXECUTABLE"
    exit 1
fi

# 检查Valgrind抑制文件是否存在
if [ ! -f "$VALGRIND_SUPPRESSION_FILE" ]; then
    echo "警告: Valgrind抑制文件不存在: $VALGRIND_SUPPRESSION_FILE"
    echo "将不使用抑制文件。"
    SUPPRESSION_ARG=""
else
    SUPPRESSION_ARG="--suppressions=$VALGRIND_SUPPRESSION_FILE"
fi

echo "XC项目 - 内存泄漏检测"
echo "=========================="
echo "可执行文件: $EXECUTABLE"
echo "报告文件: $REPORT_FILE"
echo "开始时间: $(date)"
echo

# 运行Valgrind内存检查
echo "正在运行Valgrind内存检查..."
valgrind --tool=memcheck \
         --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         $SUPPRESSION_ARG \
         --log-file="$REPORT_FILE" \
         "$EXECUTABLE" "$@"

# 检查Valgrind退出状态
VALGRIND_EXIT_CODE=$?
if [ $VALGRIND_EXIT_CODE -ne 0 ]; then
    echo "警告: Valgrind以非零状态退出: $VALGRIND_EXIT_CODE"
fi

echo
echo "内存检查完成!"
echo "报告已保存到: $REPORT_FILE"
echo "结束时间: $(date)"

# 分析报告中的泄漏
LEAK_COUNT=$(grep -c "definitely lost:" "$REPORT_FILE" || true)
if [ "$LEAK_COUNT" -gt 0 ]; then
    echo "发现内存泄漏! 请查看报告文件获取详细信息。"
    grep -A 2 "definitely lost:" "$REPORT_FILE"
else
    echo "未发现明确的内存泄漏。"
fi

exit $VALGRIND_EXIT_CODE
EOF
    
    chmod +x "$LEAK_CHECK_SCRIPT"
    echo -e "${GREEN}内存泄漏检测脚本已创建: $LEAK_CHECK_SCRIPT${NC}"
}

# 创建CPU分析脚本
create_cpu_profile_script() {
    echo -e "${YELLOW}创建CPU分析脚本...${NC}"
    
    CPU_PROFILE_SCRIPT="$SCRIPTS_DIR/run_cpu_profile.sh"
    
    cat > "$CPU_PROFILE_SCRIPT" << 'EOF'
#!/bin/bash
# XC项目 - CPU性能分析脚本

set -e  # 遇到错误立即退出

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT_DIR="$PROJECT_ROOT/reports"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="$REPORT_DIR/cpu_profile_$TIMESTAMP.txt"

# 创建报告目录
mkdir -p "$REPORT_DIR"

# 检查参数
if [ $# -lt 1 ]; then
    echo "用法: $0 <可执行文件> [参数...]"
    echo "例如: $0 bin/test_xc.exe"
    exit 1
fi

EXECUTABLE="$1"
shift  # 移除第一个参数，剩余的是传递给可执行文件的参数

# 检查可执行文件是否存在
if [ ! -f "$EXECUTABLE" ]; then
    echo "错误: 可执行文件不存在: $EXECUTABLE"
    exit 1
fi

echo "XC项目 - CPU性能分析"
echo "========================"
echo "可执行文件: $EXECUTABLE"
echo "报告文件: $REPORT_FILE"
echo "开始时间: $(date)"
echo

# 运行Valgrind Callgrind工具进行CPU分析
echo "正在运行Valgrind Callgrind进行CPU分析..."
valgrind --tool=callgrind \
         --callgrind-out-file="$REPORT_FILE" \
         "$EXECUTABLE" "$@"

# 检查Valgrind退出状态
VALGRIND_EXIT_CODE=$?
if [ $VALGRIND_EXIT_CODE -ne 0 ]; then
    echo "警告: Valgrind以非零状态退出: $VALGRIND_EXIT_CODE"
fi

echo
echo "CPU分析完成!"
echo "报告已保存到: $REPORT_FILE"
echo "结束时间: $(date)"

# 如果安装了callgrind_annotate，生成可读报告
ANNOTATE_FILE="${REPORT_FILE}.annotated.txt"
if command -v callgrind_annotate &> /dev/null; then
    echo "生成注释报告..."
    callgrind_annotate "$REPORT_FILE" > "$ANNOTATE_FILE"
    echo "注释报告已保存到: $ANNOTATE_FILE"
    
    # 显示前10个最耗时的函数
    echo
    echo "前10个最耗时的函数:"
    callgrind_annotate --auto=yes --inclusive=yes "$REPORT_FILE" | grep -A 11 "PROGRAM TOTALS"
else
    echo "未找到callgrind_annotate工具。无法生成注释报告。"
    echo "请使用KCachegrind或类似工具查看报告文件。"
fi

exit $VALGRIND_EXIT_CODE
EOF
    
    chmod +x "$CPU_PROFILE_SCRIPT"
    echo -e "${GREEN}CPU分析脚本已创建: $CPU_PROFILE_SCRIPT${NC}"
}

# 创建性能基准测试脚本
create_benchmark_script() {
    echo -e "${YELLOW}创建性能基准测试脚本...${NC}"
    
    BENCHMARK_SCRIPT="$SCRIPTS_DIR/run_benchmark.sh"
    
    cat > "$BENCHMARK_SCRIPT" << 'EOF'
#!/bin/bash
# XC项目 - 性能基准测试脚本

set -e  # 遇到错误立即退出

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT_DIR="$PROJECT_ROOT/reports"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="$REPORT_DIR/benchmark_$TIMESTAMP.txt"

# 创建报告目录
mkdir -p "$REPORT_DIR"

# 测试次数
ITERATIONS=5

# 基准测试函数
run_benchmark() {
    local test_name="$1"
    local command="$2"
    
    echo "运行基准测试: $test_name"
    echo "命令: $command"
    echo "迭代次数: $ITERATIONS"
    echo
    
    echo "[$test_name]" >> "$REPORT_FILE"
    echo "Command: $command" >> "$REPORT_FILE"
    echo "Iterations: $ITERATIONS" >> "$REPORT_FILE"
    echo "Results:" >> "$REPORT_FILE"
    
    # 运行多次测试并记录时间
    total_time=0
    for i in $(seq 1 $ITERATIONS); do
        echo "迭代 $i/$ITERATIONS..."
        start_time=$(date +%s.%N)
        eval "$command"
        end_time=$(date +%s.%N)
        
        # 计算执行时间（秒）
        execution_time=$(echo "$end_time - $start_time" | bc)
        total_time=$(echo "$total_time + $execution_time" | bc)
        
        echo "  迭代 $i: $execution_time 秒"
        echo "  Iteration $i: $execution_time seconds" >> "$REPORT_FILE"
    done
    
    # 计算平均时间
    average_time=$(echo "scale=6; $total_time / $ITERATIONS" | bc)
    echo
    echo "平均执行时间: $average_time 秒"
    echo "Average execution time: $average_time seconds" >> "$REPORT_FILE"
    echo >> "$REPORT_FILE"
}

echo "XC项目 - 性能基准测试"
echo "=========================="
echo "报告文件: $REPORT_FILE"
echo "开始时间: $(date)"
echo

# 记录系统信息
echo "系统信息:" | tee -a "$REPORT_FILE"
echo "----------" | tee -a "$REPORT_FILE"
echo "日期: $(date)" | tee -a "$REPORT_FILE"
echo "主机名: $(hostname)" | tee -a "$REPORT_FILE"
echo "操作系统: $(uname -a)" | tee -a "$REPORT_FILE"
if [ -f /proc/cpuinfo ]; then
    echo "CPU: $(grep "model name" /proc/cpuinfo | head -1 | cut -d ":" -f2 | sed 's/^[ \t]*//')" | tee -a "$REPORT_FILE"
elif [ "$(uname)" = "Darwin" ]; then
    echo "CPU: $(sysctl -n machdep.cpu.brand_string)" | tee -a "$REPORT_FILE"
fi
echo | tee -a "$REPORT_FILE"

# 运行各种基准测试
echo "开始基准测试..." | tee -a "$REPORT_FILE"
echo "----------------" | tee -a "$REPORT_FILE"

# 基本类型操作测试
run_benchmark "基本类型操作" "$PROJECT_ROOT/bin/test_xc.exe basic_types"

# 数组操作测试
run_benchmark "数组操作" "$PROJECT_ROOT/bin/test_xc.exe array_ops"

# 对象操作测试
run_benchmark "对象操作" "$PROJECT_ROOT/bin/test_xc.exe object_ops"

# 函数调用测试
run_benchmark "函数调用" "$PROJECT_ROOT/bin/test_xc.exe function_calls"

# 垃圾回收测试
run_benchmark "垃圾回收" "$PROJECT_ROOT/bin/test_xc.exe gc_test"

echo
echo "基准测试完成!"
echo "报告已保存到: $REPORT_FILE"
echo "结束时间: $(date)"
EOF
    
    chmod +x "$BENCHMARK_SCRIPT"
    echo -e "${GREEN}性能基准测试脚本已创建: $BENCHMARK_SCRIPT${NC}"
}

# 创建性能报告生成脚本
create_report_script() {
    echo -e "${YELLOW}创建性能报告生成脚本...${NC}"
    
    REPORT_SCRIPT="$SCRIPTS_DIR/generate_performance_report.sh"
    
    cat > "$REPORT_SCRIPT" << 'EOF'
#!/bin/bash
# XC项目 - 性能报告生成脚本

set -e  # 遇到错误立即退出

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT_DIR="$PROJECT_ROOT/reports"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
FINAL_REPORT="$REPORT_DIR/performance_report_$TIMESTAMP.md"

# 创建报告目录
mkdir -p "$REPORT_DIR"

echo "XC项目 - 性能报告生成"
echo "=========================="
echo "报告文件: $FINAL_REPORT"
echo "开始时间: $(date)"
echo

# 检查报告目录是否存在报告文件
if [ ! -d "$REPORT_DIR" ] || [ -z "$(ls -A "$REPORT_DIR" 2>/dev/null)" ]; then
    echo "错误: 未找到性能报告文件。请先运行性能测试。"
    exit 1
fi

# 创建报告文件
cat > "$FINAL_REPORT" << 'EOT'
# XC项目性能报告

**生成时间:** $(date)

## 系统信息

- **主机名:** $(hostname)
- **操作系统:** $(uname -a)
EOT

if [ -f /proc/cpuinfo ]; then
    echo "- **CPU:** $(grep "model name" /proc/cpuinfo | head -1 | cut -d ":" -f2 | sed 's/^[ \t]*//')" >> "$FINAL_REPORT"
elif [ "$(uname)" = "Darwin" ]; then
    echo "- **CPU:** $(sysctl -n machdep.cpu.brand_string)" >> "$FINAL_REPORT"
fi

echo "- **内存:** $(free -h 2>/dev/null | grep Mem | awk '{print $2}' || echo "未知")" >> "$FINAL_REPORT"

echo >> "$FINAL_REPORT"
echo "## 性能测试结果" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"

# 添加基准测试结果
echo "### 基准测试" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"

# 查找最新的基准测试报告
LATEST_BENCHMARK=$(ls -t "$REPORT_DIR"/benchmark_*.txt 2>/dev/null | head -1)
if [ -n "$LATEST_BENCHMARK" ]; then
    echo "找到基准测试报告: $LATEST_BENCHMARK"
    echo "添加到最终报告..."
    
    echo "**测试时间:** $(head -3 "$LATEST_BENCHMARK" | grep "日期:" | cut -d ":" -f2- | sed 's/^[ \t]*//')" >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    echo "| 测试名称 | 平均执行时间 (秒) |" >> "$FINAL_REPORT"
    echo "|---------|-----------------|" >> "$FINAL_REPORT"
    
    # 解析基准测试结果
    while IFS= read -r line; do
        if [[ "$line" == \[*\] ]]; then
            test_name=$(echo "$line" | tr -d '[]')
            in_test=true
        elif [[ "$in_test" == true && "$line" == "Average execution time:"* ]]; then
            avg_time=$(echo "$line" | cut -d ":" -f2 | sed 's/^[ \t]*//' | sed 's/ seconds//')
            echo "| $test_name | $avg_time |" >> "$FINAL_REPORT"
            in_test=false
        fi
    done < "$LATEST_BENCHMARK"
    
    echo >> "$FINAL_REPORT"
else
    echo "未找到基准测试报告。" >> "$FINAL_REPORT"
fi

# 添加内存分析结果
echo "### 内存分析" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"

# 查找最新的内存报告
LATEST_MEMORY=$(ls -t "$REPORT_DIR"/memory_report_*.txt 2>/dev/null | head -1)
if [ -n "$LATEST_MEMORY" ]; then
    echo "找到内存分析报告: $LATEST_MEMORY"
    echo "添加到最终报告..."
    
    echo "**分析时间:** $(grep -m 1 "开始时间:" "$LATEST_MEMORY" | cut -d ":" -f2- | sed 's/^[ \t]*//')" >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    
    # 提取内存泄漏摘要
    echo "#### 内存泄漏摘要" >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    echo '```' >> "$FINAL_REPORT"
    grep -A 2 "LEAK SUMMARY" "$LATEST_MEMORY" >> "$FINAL_REPORT" || echo "未找到内存泄漏摘要。" >> "$FINAL_REPORT"
    echo '```' >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    
    # 提取明确的内存泄漏
    echo "#### 明确的内存泄漏" >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    echo '```' >> "$FINAL_REPORT"
    grep -A 10 "definitely lost:" "$LATEST_MEMORY" >> "$FINAL_REPORT" || echo "未找到明确的内存泄漏。" >> "$FINAL_REPORT"
    echo '```' >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
else
    echo "未找到内存分析报告。" >> "$FINAL_REPORT"
fi

# 添加CPU分析结果
echo "### CPU分析" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"

# 查找最新的CPU分析报告
LATEST_CPU=$(ls -t "$REPORT_DIR"/cpu_profile_*.txt.annotated.txt 2>/dev/null | head -1)
if [ -n "$LATEST_CPU" ]; then
    echo "找到CPU分析报告: $LATEST_CPU"
    echo "添加到最终报告..."
    
    echo "**分析时间:** $(grep -m 1 "开始时间:" "$LATEST_CPU" | cut -d ":" -f2- | sed 's/^[ \t]*//')" >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    
    # 提取前10个最耗时的函数
    echo "#### 前10个最耗时的函数" >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
    echo '```' >> "$FINAL_REPORT"
    grep -A 11 "PROGRAM TOTALS" "$LATEST_CPU" >> "$FINAL_REPORT" || echo "未找到CPU分析数据。" >> "$FINAL_REPORT"
    echo '```' >> "$FINAL_REPORT"
    echo >> "$FINAL_REPORT"
else
    echo "未找到CPU分析报告。" >> "$FINAL_REPORT"
fi

# 添加结论和建议
echo "## 结论和建议" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"
echo "根据性能测试结果，提出以下改进建议：" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"
echo "1. **待添加**: 根据实际测试结果添加具体建议" >> "$FINAL_REPORT"
echo "2. **待添加**: 根据实际测试结果添加具体建议" >> "$FINAL_REPORT"
echo "3. **待添加**: 根据实际测试结果添加具体建议" >> "$FINAL_REPORT"
echo >> "$FINAL_REPORT"

echo "性能报告生成完成!"
echo "报告已保存到: $FINAL_REPORT"
echo "结束时间: $(date)"
EOF
    
    chmod +x "$REPORT_SCRIPT"
    echo -e "${GREEN}性能报告生成脚本已创建: $REPORT_SCRIPT${NC}"
}

# 主函数
main() {
    # 检查Valgrind是否已安装
    if command -v valgrind &> /dev/null; then
        VALGRIND_VERSION=$(valgrind --version)
        echo -e "${GREEN}Valgrind已安装: $VALGRIND_VERSION${NC}"
    else
        install_valgrind
    fi
    
    # 验证Valgrind安装
    verify_valgrind
    
    # 创建配置和脚本
    create_suppression_file
    create_leak_check_script
    create_cpu_profile_script
    create_benchmark_script
    create_report_script
    
    echo -e "${GREEN}Valgrind安装和配置完成!${NC}"
    echo "可以使用以下命令进行性能分析:"
    echo "  - 内存泄漏检测: $SCRIPTS_DIR/run_memory_check.sh bin/test_xc.exe"
    echo "  - CPU性能分析: $SCRIPTS_DIR/run_cpu_profile.sh bin/test_xc.exe"
    echo "  - 性能基准测试: $SCRIPTS_DIR/run_benchmark.sh"
    echo "  - 生成性能报告: $SCRIPTS_DIR/generate_performance_report.sh"
}

# 执行主函数
main 