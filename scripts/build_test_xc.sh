#!/bin/bash

# 构建和运行XC测试程序的脚本

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录的上级目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 设置源代码和输出目录
SRC_DIR="${PROJECT_ROOT}/src"
BIN_DIR="${PROJECT_ROOT}/bin"
TEST_DIR="${PROJECT_ROOT}/test"

# 设置编译器
COSMOCC="${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/bin/cosmocc"

# 设置编译选项
CFLAGS="-Os -fomit-frame-pointer -fno-pie -fno-pic -fno-common -fno-plt -mcmodel=large -finline-functions -I${PROJECT_ROOT}/src -I${PROJECT_ROOT}/include -I${SRC_DIR} -I${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/include"
LDFLAGS="-static -Wl,--gc-sections -Wl,--build-id=none"

# 创建bin目录（如果不存在）
mkdir -p "${BIN_DIR}"
mkdir -p "${TEST_DIR}"

# 编译XC测试程序
echo "编译XC测试程序..."

# 编译源文件列表（按依赖顺序排序）
SOURCE_FILES=(
    # infrax层
    "${SRC_DIR}/infrax/InfraxCore.c"
    "${SRC_DIR}/infrax/InfraxMemory.c"
    "${SRC_DIR}/infrax/InfraxLog.c"
    "${SRC_DIR}/infrax/InfraxThread.c"
    "${SRC_DIR}/infrax/InfraxSync.c"
    "${SRC_DIR}/infrax/InfraxAsync.c"
    "${SRC_DIR}/infrax/InfraxNet.c"
    
    # 核心运行时
    "${SRC_DIR}/xc/xc.c"
    "${SRC_DIR}/xc/xc_init.c"
    "${SRC_DIR}/xc/xc_string.c"
    "${SRC_DIR}/xc/xc_null.c"
    "${SRC_DIR}/xc/xc_boolean.c"
    "${SRC_DIR}/xc/xc_number.c"
    "${SRC_DIR}/xc/xc_function.c"
    "${SRC_DIR}/xc/xc_array.c"
    "${SRC_DIR}/xc/xc_object.c"
    "${SRC_DIR}/xc/xc_error.c"
    "${SRC_DIR}/xc/xc_vm.c"
    "${SRC_DIR}/xc/xc_std_console.c"
    "${SRC_DIR}/xc/xc_std_math.c"
    
    # 测试框架
    "${SRC_DIR}/xc/test_utils.c"
    
    # 测试套件
    "${SRC_DIR}/xc/test_xc_types.c"
    "${SRC_DIR}/xc/test_xc_gc.c"
    "${SRC_DIR}/xc/test_xc_exception.c"
    "${SRC_DIR}/xc/test_xc_composite_types.c"
    "${SRC_DIR}/xc/test_xc.c"
)

# 编译所有源文件
"${COSMOCC}" ${CFLAGS} "${SOURCE_FILES[@]}" ${LDFLAGS} -o "${BIN_DIR}/test_xc.exe"

# 显示文件信息
echo -e "\n生成的可执行文件信息:"
file "${BIN_DIR}/test_xc.exe"

# 复制到测试目录以便测试
cp "${BIN_DIR}/test_xc.exe" "${TEST_DIR}/"

# 运行测试程序
echo "运行测试程序: ${TEST_DIR}/test_xc.exe"
"${TEST_DIR}/test_xc.exe"
