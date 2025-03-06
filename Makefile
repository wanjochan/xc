# XC项目Makefile

# ~/cosmocc/bin/make

# 项目根目录
PROJECT_ROOT := $(shell pwd)

# 目录设置
SRC_DIR := $(PROJECT_ROOT)/src
INCLUDE_DIR := $(PROJECT_ROOT)/include
LIB_DIR := $(PROJECT_ROOT)/lib
BIN_DIR := $(PROJECT_ROOT)/bin
TEST_DIR := $(PROJECT_ROOT)/test
SCRIPTS_DIR := $(PROJECT_ROOT)/scripts

# 测试目录
INTERNAL_TEST_DIR := $(TEST_DIR)/internal
EXTERNAL_TEST_DIR := $(TEST_DIR)/external

# 编译器设置
COSMOCC := ~/cosmocc/bin/cosmocc
AR := ~/cosmocc/bin/cosmoar

# 编译选项
CFLAGS := -Os -fomit-frame-pointer -fno-pie -fno-pic -fno-common -fno-plt -mcmodel=large -finline-functions
INCLUDES := -I$(SRC_DIR) -I$(SRC_DIR)/infrax -I$(INCLUDE_DIR) -I~/cosmocc/include
LDFLAGS := -static -Wl,--gc-sections -Wl,--build-id=none
#LDFLAGS := -static -Wl,--whole-archive -lxc -Wl,--no-whole-archive

# 默认目标
.PHONY: all
all: libxc test

# 创建目录
.PHONY: dirs
dirs:
	@mkdir -p $(INCLUDE_DIR) $(LIB_DIR) $(BIN_DIR) $(TEST_DIR) $(INTERNAL_TEST_DIR) $(EXTERNAL_TEST_DIR)

# 构建libxc.a
.PHONY: libxc
libxc: dirs
	@echo "构建libxc.a..."
	@bash $(SCRIPTS_DIR)/build_libxc.sh

# 构建并运行测试
.PHONY: test
test: test-internal test-external

# 构建并运行内部测试
.PHONY: test-internal
test-internal: dirs libxc
	@echo "构建并运行内部测试..."
	@bash $(SCRIPTS_DIR)/run_internal_tests.sh

# 构建并运行外部测试
.PHONY: test-external
test-external: dirs libxc
	@echo "构建并运行外部测试..."
	@bash $(SCRIPTS_DIR)/run_external_tests.sh

# 清理构建产物
.PHONY: clean
clean:
	@echo "清理构建产物..."
	@rm -f $(SRC_DIR)/xc/*.o
	@rm -f $(SRC_DIR)/infrax/*.o
	@rm -f $(LIB_DIR)/*.a
	@rm -f $(BIN_DIR)/*.exe
	@rm -f $(TEST_DIR)/*.exe
	@rm -f $(INTERNAL_TEST_DIR)/*.o
	@rm -f $(EXTERNAL_TEST_DIR)/*.o
	@echo "清理完成"

# 创建GitHub发布
.PHONY: github_release
github_release: libxc
	@echo "创建GitHub发布..."
	@if [ -z "$(VERSION)" ]; then \
		echo "错误: 缺少版本号参数"; \
		echo "使用方法: make github_release VERSION=版本号 [NOTES=\"发布说明\"]"; \
		echo "例如: make github_release VERSION=v1.0.0 NOTES=\"首次正式发布\""; \
		exit 1; \
	fi
	@bash $(SCRIPTS_DIR)/github_release.sh "$(VERSION)" "$(NOTES)"

# 帮助信息
.PHONY: help
help:
	@echo "XC项目构建系统"
	@echo "可用目标:"
	@echo "  all            - 构建libxc.a和测试程序（默认）"
	@echo "  libxc          - 只构建libxc.a静态库"
	@echo "  test           - 构建并运行所有测试程序"
	@echo "  test-internal  - 构建并运行内部测试程序"
	@echo "  test-external  - 构建并运行外部测试程序"
	@echo "  clean          - 清理所有构建产物"
	@echo "  github_release - 创建GitHub发布包并发布"
	@echo "                   使用方法: make github_release VERSION=版本号 [NOTES=\"发布说明\"]"
	@echo "                   例如: make github_release VERSION=v1.0.0 NOTES=\"首次正式发布\""
	@echo "  help           - 显示此帮助信息"
