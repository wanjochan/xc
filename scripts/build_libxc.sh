#!/bin/bash

# 构建libxc.a的脚本

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录的上级目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 设置源代码和输出目录
SRC_DIR="${PROJECT_ROOT}/src"
INCLUDE_DIR="${PROJECT_ROOT}/include"
LIB_DIR="${PROJECT_ROOT}/lib"

# 设置编译器
COSMOCC="${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/bin/cosmocc"
AR="${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/bin/ar"

# 设置编译选项
CFLAGS="-Os -fomit-frame-pointer -fno-pie -fno-pic -fno-common -fno-plt -mcmodel=large -finline-functions -I${SRC_DIR} -I${INCLUDE_DIR} -I${PROJECT_ROOT}/../Downloads/cosmocc-4.0.2/include"

# 创建输出目录（如果不存在）
mkdir -p "${LIB_DIR}"
mkdir -p "${INCLUDE_DIR}"

# 编译XC源文件
echo "编译XC源文件..."

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
)

# 编译所有源文件为目标文件
OBJECT_FILES=()
for src in "${SOURCE_FILES[@]}"; do
    obj="${src%.c}.o"
    echo "编译 ${src}..."
    "${COSMOCC}" ${CFLAGS} -c "${src}" -o "${obj}"
    OBJECT_FILES+=("${obj}")
done

# 创建静态库
echo "创建 libxc.a..."
"${AR}" rcs "${LIB_DIR}/libxc.a" "${OBJECT_FILES[@]}"

# 复制头文件到include目录
echo "复制头文件到include目录..."
cp "${SRC_DIR}/xc/xc.h" "${INCLUDE_DIR}/"
cp "${SRC_DIR}/xc/xc_std_console.h" "${INCLUDE_DIR}/"
cp "${SRC_DIR}/xc/xc_std_math.h" "${INCLUDE_DIR}/"

# 创建一个合并的libxc.h头文件
echo "创建合并的libxc.h头文件..."
cat > "${INCLUDE_DIR}/libxc.h" << EOF
/*
 * libxc.h - XC运行时库的主头文件
 */
#ifndef LIBXC_H
#define LIBXC_H

#include "xc.h"
#include "xc_std_console.h"
#include "xc_std_math.h"

#endif /* LIBXC_H */
EOF

# 显示文件信息
echo -e "\n生成的静态库信息:"
ls -la "${LIB_DIR}/libxc.a"

echo "libxc.a构建完成!"
