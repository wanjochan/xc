#!/bin/bash

# 运行XC内部测试（白盒测试）的脚本

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录的上级目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 设置目录
SRC_DIR="${PROJECT_ROOT}/src"
INCLUDE_DIR="${PROJECT_ROOT}/include"
LIB_DIR="${PROJECT_ROOT}/lib"
BIN_DIR="${PROJECT_ROOT}/bin"
TEST_DIR="${PROJECT_ROOT}/test"
INTERNAL_TEST_DIR="${TEST_DIR}/internal"

# 设置编译器
COSMOCC=~/cosmocc/bin/cosmocc

# 设置编译选项
CFLAGS="-Os -g -I${SRC_DIR} -I${SRC_DIR}/infrax -I${INCLUDE_DIR} -I${INTERNAL_TEST_DIR} -I~/cosmocc/include"

# 创建输出目录（如果不存在）
mkdir -p "${BIN_DIR}"

# 清理旧的测试二进制文件
rm -f "${BIN_DIR}/test_internal.exe"

# 编译内部测试
echo "run_internal_tests.sh: 编译XC内部测试程序..."

# 编译测试框架
${COSMOCC} ${CFLAGS} -c "${INTERNAL_TEST_DIR}/test_utils.c" -o "${INTERNAL_TEST_DIR}/test_utils.o"

# 编译测试文件
TEST_FILES=(
    "${INTERNAL_TEST_DIR}/test_xc_exception.c"
    "${INTERNAL_TEST_DIR}/test_xc_types.c"
    "${INTERNAL_TEST_DIR}/test_xc.c"
    "${INTERNAL_TEST_DIR}/test_xc_array.c"
    "${INTERNAL_TEST_DIR}/test_xc_object.c"
)

for TEST_FILE in "${TEST_FILES[@]}"; do
    echo "run_internal_tests.sh: 编译内部测试文件: ${TEST_FILE}"
    TEST_BASENAME=$(basename "${TEST_FILE}" .c)
    ${COSMOCC} ${CFLAGS} -c "${TEST_FILE}" -o "${INTERNAL_TEST_DIR}/${TEST_BASENAME}.o"
done

# 链接测试程序
echo "run_internal_tests.sh: 链接内部测试程序..."
${COSMOCC} -o "${BIN_DIR}/test_internal.exe" \
    "${INTERNAL_TEST_DIR}/test_utils.o" \
    "${INTERNAL_TEST_DIR}/test_xc_exception.o" \
    "${INTERNAL_TEST_DIR}/test_xc_types.o" \
    "${INTERNAL_TEST_DIR}/test_xc.o" \
    "${INTERNAL_TEST_DIR}/test_xc_array.o" \
    "${INTERNAL_TEST_DIR}/test_xc_object.o" \
    "${LIB_DIR}/libxc.a"

# 显示编译结果
echo -e "\nrun_internal_tests.sh: 生成的内部测试可执行文件:"
ls -la "${BIN_DIR}/test_internal.exe"

# 运行测试
echo -e "\nrun_internal_tests.sh: 运行内部测试程序: ${BIN_DIR}/test_internal.exe\n"
"${BIN_DIR}/test_internal.exe"

echo -e "\nrun_internal_tests.sh: 内部测试结束!" 