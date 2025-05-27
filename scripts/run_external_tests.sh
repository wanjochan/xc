#!/bin/bash

# 运行XC外部测试（黑盒测试）的脚本

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录的上级目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 设置目录
INCLUDE_DIR="${PROJECT_ROOT}/include"
LIB_DIR="${PROJECT_ROOT}/lib"
BIN_DIR="${PROJECT_ROOT}/bin"
TEST_DIR="${PROJECT_ROOT}/test"
EXTERNAL_TEST_DIR="${TEST_DIR}/external"

# 设置编译器，可通过环境变量覆盖
COSMOCC="${COSMOCC:-$(command -v cosmocc 2>/dev/null || command -v cc)}"

# 设置编译选项 - 注意这里只包含公共头文件目录
CFLAGS="-Os -g -I${INCLUDE_DIR} -I${EXTERNAL_TEST_DIR}"

# 创建输出目录（如果不存在）
mkdir -p "${BIN_DIR}"

# 清理旧的测试二进制文件
rm -f "${BIN_DIR}/test_external.exe"

# 编译外部测试
echo "run_external_tests.sh: 编译XC外部测试程序..."

# 编译测试框架
${COSMOCC} ${CFLAGS} -c "${EXTERNAL_TEST_DIR}/test_utils.c" -o "${EXTERNAL_TEST_DIR}/test_utils.o"

# 编译测试文件（不包含main函数）
TEST_FILES=(
    "${EXTERNAL_TEST_DIR}/test_xc_array.c"
    "${EXTERNAL_TEST_DIR}/test_xc_object.c"
    "${EXTERNAL_TEST_DIR}/test_xc_function.c"
    "${EXTERNAL_TEST_DIR}/test_xc_exception.c"
)

for TEST_FILE in "${TEST_FILES[@]}"; do
    echo "run_external_tests.sh: 编译外部测试文件: ${TEST_FILE}"
    TEST_BASENAME=$(basename "${TEST_FILE}" .c)
    ${COSMOCC} ${CFLAGS} -c "${TEST_FILE}" -o "${EXTERNAL_TEST_DIR}/${TEST_BASENAME}.o"
done

# 编译主测试文件
echo "run_external_tests.sh: 编译主测试文件: ${EXTERNAL_TEST_DIR}/test_xc_main.c"
${COSMOCC} ${CFLAGS} -c "${EXTERNAL_TEST_DIR}/test_xc_main.c" -o "${EXTERNAL_TEST_DIR}/test_xc_main.o"

# 链接测试程序
echo "run_external_tests.sh: 链接外部测试程序..."
${COSMOCC} -o "${BIN_DIR}/test_external.exe" \
    "${EXTERNAL_TEST_DIR}/test_utils.o" \
    "${EXTERNAL_TEST_DIR}/test_xc_array.o" \
    "${EXTERNAL_TEST_DIR}/test_xc_object.o" \
    "${EXTERNAL_TEST_DIR}/test_xc_function.o" \
    "${EXTERNAL_TEST_DIR}/test_xc_exception.o" \
    "${EXTERNAL_TEST_DIR}/test_xc_main.o" \
    -L${LIB_DIR} -lxc

# 显示编译结果
echo -e "\nrun_external_tests.sh: 生成的外部测试可执行文件:"
ls -la "${BIN_DIR}/test_external.exe"

# 运行测试
echo -e "\nrun_external_tests.sh: 运行外部测试程序: ${BIN_DIR}/test_external.exe\n"
"${BIN_DIR}/test_external.exe"

echo -e "\nrun_external_tests.sh: 外部测试结束!"
