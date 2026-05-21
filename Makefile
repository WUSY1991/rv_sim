# RV_sim Makefile - RISC-V RV32IMACF 模拟器
# 模块化版本：取指、译码、执行、写回分离

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# 目标文件
TARGET = rv_sim.exe

# 源文件
SRCS = main.c cpu.c mem.c fetch.c decode.c execute.c writeback.c
OBJS = $(SRCS:.c=.o)

# 头文件
HDRS = cpu.h mem.h config.h

.PHONY: all clean run test debug help

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(addprefix output/, $(OBJS)) $(LDFLAGS)

# 编译规则
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $(addprefix output/, $(@))

# 清理
clean:
	rm -rf $(TARGET) output/*

# 运行测试
run: $(TARGET)
	./$(TARGET)

# 测试
test: $(TARGET)
	@echo "========================================"
	@echo "  运行 RV32IMACF 指令集测试"
	@echo "========================================"
	./$(TARGET)
	@echo ""
	@echo "========================================"
	@echo "  测试完成"
	@echo "========================================"

# 调试版本
debug: CFLAGS += -g -DDEBUG_TRACE
debug: clean $(TARGET)

# 生成 FPU 测试二进制
test_fpu: test_fpu.c
	$(CC) -o test_fpu test_fpu.c -lm
	./test_fpu

# 帮助
help:
	@echo "RV_sim - RISC-V RV32IMACF 模拟器"
	@echo ""
	@echo "构建选项:"
	@echo "  make        - 编译项目"
	@echo "  make run    - 编译并运行测试程序"
	@echo "  make test   - 运行完整测试"
	@echo "  make clean  - 清理构建文件"
	@echo "  make debug  - 编译调试版本 (带跟踪输出)"
	@echo "  make test_fpu - 编译并运行 FPU 测试生成器"
	@echo ""
	@echo "支持的指令集:"
	@echo "  - I: 基础整数指令 (ADD, SUB, AND, OR, 等)"
	@echo "  - M: 乘除扩展 (MUL, DIV, REM, 等)"
	@echo "  - A: 原子操作 (LR, SC, AMO, 等)"
	@echo "  - F: 单精度浮点 (FADD, FMUL, FMADD, 等)"
