# XC项目Makefile

# 项目根目录
PROJECT_ROOT := $(shell pwd)

# 目录设置
SRC_DIR := $(PROJECT_ROOT)/src
INCLUDE_DIR := $(PROJECT_ROOT)/include
LIB_DIR := $(PROJECT_ROOT)/lib
BIN_DIR := $(PROJECT_ROOT)/bin
TEST_DIR := $(PROJECT_ROOT)/test
SCRIPTS_DIR := $(PROJECT_ROOT)/scripts

# 编译器设置
COSMOCC := $(PROJECT_ROOT)/../Downloads/cosmocc-4.0.2/bin/cosmocc
AR := $(PROJECT_ROOT)/../Downloads/cosmocc-4.0.2/bin/ar

# 编译选项
CFLAGS := -Os -fomit-frame-pointer -fno-pie -fno-pic -fno-common -fno-plt -mcmodel=large -finline-functions
INCLUDES := -I$(SRC_DIR) -I$(SRC_DIR)/infrax -I$(INCLUDE_DIR) -I$(PROJECT_ROOT)/../Downloads/cosmocc-4.0.2/include
LDFLAGS := -static -Wl,--gc-sections -Wl,--build-id=none

# 默认目标
.PHONY: all
all: libxc test

# 创建目录
.PHONY: dirs
dirs:
	@mkdir -p $(INCLUDE_DIR) $(LIB_DIR) $(BIN_DIR) $(TEST_DIR)

# 构建libxc.a
.PHONY: libxc
libxc: dirs
	@echo "构建libxc.a..."
	@bash $(SCRIPTS_DIR)/build_libxc.sh

# 构建并运行测试
.PHONY: test
test: dirs
	@echo "构建并运行测试..."
	@bash $(SCRIPTS_DIR)/build_test_xc.sh

# 清理构建产物
.PHONY: clean
clean:
	@echo "清理构建产物..."
	@rm -f $(SRC_DIR)/xc/*.o
	@rm -f $(SRC_DIR)/infrax/*.o
	@rm -f $(LIB_DIR)/*.a
	@rm -f $(BIN_DIR)/*.exe
	@rm -f $(TEST_DIR)/*.exe
	@echo "清理完成"

# 帮助信息
.PHONY: help
help:
	@echo "XC项目构建系统"
	@echo "可用目标:"
	@echo "  all      - 构建libxc.a和测试程序（默认）"
	@echo "  libxc    - 只构建libxc.a静态库"
	@echo "  test     - 构建并运行测试程序"
	@echo "  clean    - 清理所有构建产物"
	@echo "  help     - 显示此帮助信息"
