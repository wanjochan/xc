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
TMP_DIR="${PROJECT_ROOT}/tmp"

# 设置编译器
COSMOCC=~/cosmocc/bin/cosmocc
AR=~/cosmocc/bin/cosmoar

# 设置编译选项
CFLAGS="-Os -fomit-frame-pointer -fno-pie -fno-pic -fno-common -fno-plt -mcmodel=large -finline-functions -I${SRC_DIR} -I${SRC_DIR}/infrax -I${INCLUDE_DIR} -I~/cosmocc/include"

# 创建输出目录（如果不存在）
mkdir -p "${LIB_DIR}"
mkdir -p "${INCLUDE_DIR}"
mkdir -p "${TMP_DIR}"

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
    "${SRC_DIR}/xc/xc_gc.c"
    "${SRC_DIR}/xc/xc_exception.c"
    
    # 类型系统
    "${SRC_DIR}/xc/xc_types/xc_null.c"
    "${SRC_DIR}/xc/xc_types/xc_boolean.c"
    "${SRC_DIR}/xc/xc_types/xc_number.c"
    "${SRC_DIR}/xc/xc_types/xc_string.c"
    "${SRC_DIR}/xc/xc_types/xc_function.c"
    "${SRC_DIR}/xc/xc_types/xc_array.c"
    "${SRC_DIR}/xc/xc_types/xc_object.c"
    "${SRC_DIR}/xc/xc_types/xc_compare.c"
    
    # 错误处理和虚拟机
    "${SRC_DIR}/xc/xc_error.c"
    "${SRC_DIR}/xc/xc_vm.c"
    
    # 标准库
    "${SRC_DIR}/xc/xc_std/xc_std_console.c"
    "${SRC_DIR}/xc/xc_std/xc_std_math.c"
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

# 创建一个临时的完整头文件
echo "生成完整的展开头文件 libxc.h..."

# 创建一个临时的汇总头文件
cat > "${TMP_DIR}/libxc_all.h" << EOF
/*
 * libxc.h - XC运行时库的公共API头文件
 */
#ifndef LIBXC_H
#define LIBXC_H

/* 包含必要的标准C库头文件 */
#include <stddef.h>  /* size_t */
#include <stdarg.h>  /* va_list */
#include <setjmp.h>  /* jmp_buf */
#include <stdbool.h> /* bool */

/* 直接包含所有需要的XC头文件 */
#include "${SRC_DIR}/xc/xc.h"
#include "${SRC_DIR}/xc/xc_types/xc_types.h"
#include "${SRC_DIR}/xc/xc_gc.h"
#include "${SRC_DIR}/xc/xc_exception.h"
#include "${SRC_DIR}/xc/xc_std/xc_std_console.h"
#include "${SRC_DIR}/xc/xc_std/xc_std_math.h"

#endif /* LIBXC_H */
EOF

# 使用预处理器展开所有头文件
"${COSMOCC}" -E -P -C \
    -I"${INCLUDE_DIR}" \
    -I"${SRC_DIR}" \
    -I"${SRC_DIR}/infrax" \
    -I~/cosmocc/include \
    "${TMP_DIR}/libxc_all.h" > "${TMP_DIR}/libxc_expanded.h"

# 提取XC相关的部分（从第195行开始）
tail -n +195 "${TMP_DIR}/libxc_expanded.h" > "${TMP_DIR}/libxc_xc_part.h"

# 添加头部注释和保护宏
cat > "${INCLUDE_DIR}/libxc.h" << EOF
/*
 * libxc.h - XC运行时库的公共API头文件 (完全展开版)
 */
#ifndef LIBXC_H
#define LIBXC_H

/* 包含必要的标准C库头文件 */
#include "cosmopolitan.h"

/* XC运行时库的公共API */
EOF

# 添加XC相关的内容
cat "${TMP_DIR}/libxc_xc_part.h" >> "${INCLUDE_DIR}/libxc.h"

# 添加结尾保护宏
echo $'\n#endif /* LIBXC_H */' >> "${INCLUDE_DIR}/libxc.h"

# 显示文件信息
echo -e "\n生成的静态库信息:"
ls -la "${LIB_DIR}/libxc.a"
ls -la "${INCLUDE_DIR}/libxc.h"

echo "libxc.a和完全展开的libxc.h构建完成!"
