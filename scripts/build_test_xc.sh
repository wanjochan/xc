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
LIB_DIR="${PROJECT_ROOT}/lib"

# 设置编译器，允许通过环境变量覆盖，并在缺少cosmocc时退回gcc/cc
COSMOCC=${COSMOCC:-"${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/bin/cosmocc"}
if [ ! -x "$COSMOCC" ]; then
    if command -v cosmocc >/dev/null 2>&1; then
        COSMOCC=$(command -v cosmocc)
    elif command -v gcc >/dev/null 2>&1; then
        COSMOCC=$(command -v gcc)
    else
        COSMOCC=$(command -v cc)
    fi
fi

# 设置编译选项
CFLAGS="-Os -fomit-frame-pointer -fno-pie -fno-pic -fno-common -fno-plt -mcmodel=large -finline-functions -I${PROJECT_ROOT}/src -I${PROJECT_ROOT}/src/infrax -I${PROJECT_ROOT}/include -I${SRC_DIR} -I${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/include"
LDFLAGS="-static -Wl,--gc-sections -Wl,--build-id=none -L${LIB_DIR} -lxc"

# 创建bin目录（如果不存在）
mkdir -p "${BIN_DIR}"
mkdir -p "${TEST_DIR}"

# 编译XC测试程序
echo "编译XC测试程序..."

# 编译测试文件列表
TEST_SOURCE_FILES=(
    # 测试框架
    "${TEST_DIR}/test_utils.c"
    
    # 测试套件
    "${TEST_DIR}/test_xc.c"
    "${TEST_DIR}/test_xc_types.c"
    "${TEST_DIR}/test_xc_gc.c"
    "${TEST_DIR}/test_xc_exception.c"
    "${TEST_DIR}/test_xc_composite_types.c"
    "${TEST_DIR}/test_xc_object.c"
    "${TEST_DIR}/test_xc_stdc.c"
    # "${TEST_DIR}/test_xc_array.c"  # 暂时注释掉，需要重写以使用公共API
)

# 编译所有测试源文件
"${COSMOCC}" ${CFLAGS} "${TEST_SOURCE_FILES[@]}" ${LDFLAGS} -o "${BIN_DIR}/test_xc.exe"

# 显示文件信息
echo -e "\n生成的可执行文件信息:"
file "${BIN_DIR}/test_xc.exe"

# 复制到测试目录以便测试
cp "${BIN_DIR}/test_xc.exe" "${TEST_DIR}/"

# 运行测试程序
echo "运行测试程序: ${TEST_DIR}/test_xc.exe"
"${TEST_DIR}/test_xc.exe"
