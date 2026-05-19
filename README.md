# RV_sim - RISC-V RV32IMACF 模拟器

纯 C 语言实现的 RISC-V 处理器模拟器，支持 **RV32IMACF** 完整指令集。

## 项目结构 (模块化)

```
RV_sim/
├── cpu.h          # CPU 核心结构体定义 (寄存器、状态、指令格式)
├── cpu.c          # CPU 初始化、加载程序、运行控制
├── fetch.c        # 取指模块 (从内存获取指令)
├── decode.c       # 译码模块 (解析指令格式)
├── execute.c      # 执行模块 (所有指令的执行逻辑)
├── writeback.c    # 写回模块 (寄存器/内存写回)
├── main.c         # 主程序入口
├── test_fpu.c     # FPU 测试程序生成器
├── Makefile       # 构建脚本
├── README.md      # 本文档
└── tests/         # 独立测试用例
    ├── Makefile
    ├── test_m_extension.c    # M 扩展 (乘除) 测试
    ├── test_f_extension.c    # F 扩展 (浮点) 测试
    ├── test_a_extension.c    # A 扩展 (原子) 测试
    └── test_base_integer.c   # 基础整数指令测试
```

### 模块职责

| 模块 | 职责 |
|------|------|
| `cpu.h` | 定义 `CPU` 结构体 (包含所有寄存器、内存、状态)，指令类型定义，常量定义 |
| `cpu.c` | CPU 初始化、程序加载、运行循环、状态打印 |
| `fetch.c` | 从内存中取指令，检查 PC 对齐和边界 |
| `decode.c` | 将 32 位指令解析为 R/I/S/R4 型结构，提取操作码/寄存器/立即数 |
| `execute.c` | 所有指令的执行逻辑 (整数/浮点/乘除/原子/分支跳转) |
| `writeback.c` | 统一的寄存器/内存写回接口 |
| `main.c` | 命令行参数解析、测试程序定义 |

## 支持的指令集

### I - 基础整数指令
- 算术：`ADD`, `SUB`, `ADDI`
- 逻辑：`AND`, `OR`, `XOR`, `ANDI`, `ORI`, `XORI`
- 移位：`SLL`, `SRL`, `SRA`, `SLLI`, `SRLI`, `SRAI`
- 比较：`SLT`, `SLTU`, `SLTI`, `SLTIU`
- 加载/存储：`LB`, `LH`, `LW`, `SB`, `SH`, `SW`
- 跳转：`JAL`, `JALR`, `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`
- 上位立即数：`LUI`, `AUIPC`
- 系统：`ECALL`

### M - 乘除扩展
- `MUL`, `MULH`, `MULHSU`, `MULHU`
- `DIV`, `DIVU`, `REM`, `REMU`

### A - 原子操作扩展
- `LR.W`, `SC.W`
- `AMOSWAP.W`, `AMOADD.W`, `AMOXOR.W`, `AMOAND.W`, `AMOOR.W`
- `AMOMIN.W`, `AMOMAX.W`, `AMOMINU.W`, `AMOMAXU.W`

### F - 单精度浮点扩展
- 算术：`FADD.S`, `FSUB.S`, `FMUL.S`, `FDIV.S`, `FSQRT.S`
- 融合乘加：`FMADD.S`, `FMSUB.S`, `FNMSUB.S`, `FNMADD.S`
- 符号操作：`FSGNJ.S`, `FSGNJN.S`, `FSGNJX.S`
- 最值：`FMIN.S`, `FMAX.S`
- 比较：`FEQ.S`, `FLT.S`, `FLE.S`
- 类型转换：`FCVT.S.W`, `FCVT.W.S`, `FCVT.S.WU`, `FCVT.WU.S`
- 分类：`FCLASS.S`
- 加载/存储：`FLW`, `FSW`

## 快速开始

### 编译

```bash
make          # 编译项目
make run      # 编译并运行测试程序
make test     # 运行完整测试
make debug    # 编译调试版本 (带指令跟踪)
make clean    # 清理构建文件
```

### 运行内置测试

```bash
./rv_sim
```

### 运行独立测试用例

```bash
cd tests
make run_all      # 运行所有测试
make run_m        # 仅运行 M 扩展测试
make run_f        # 仅运行 F 扩展测试
make run_a        # 仅运行 A 扩展测试
make run_base     # 仅运行基础整数测试
```

### 加载自定义二进制程序

```bash
./rv_sim program.bin
```

### 帮助

```bash
./rv_sim --help
```

## CPU 状态结构体

```c
typedef struct {
    uint32_t regs[NUM_REGS];      // 通用寄存器 x0-x31
    Float32 fregs[NUM_FREGS];     // 浮点寄存器 f0-f31
    uint32_t pc;                   // 程序计数器
    uint32_t memory[MEM_SIZE/4];   // 内存 (1MB, 字访问)
    uint32_t fcsr;                 // 浮点控制状态寄存器
    int has_reservation;           // 原子操作保留标志
    uint32_t reservation_addr;     // 保留地址
    int halted;                    // 停止标志
    uint32_t cycles;               // 执行周期数
} CPU;
```

## 测试程序输出示例

```
========================================
  RISC-V RV32IMACF 模拟器
========================================

开始执行 (PC=0x00000000)...

========================================
  完成 (37 周期)
========================================

=== 通用寄存器 ===
  x1  = 0x00000023 (        35)   ; 5 * 7
  x6  = 0x00000002 (         2)   ; 8 / 3
  x7  = 0x00000002 (         2)   ; 8 % 3
  x8  = 0x00000015 (        21)   ; 10 + 11
  x9  = 0xFFFFFFFF (        -1)   ; 10 - 11

=== 浮点寄存器 ===
  f0  = 0x40400000 (3.000000)
  f1  = 0x40000000 (2.000000)
  f2  = 0x3f800000 (1.000000)
  f3  = 0x40e00000 (7.000000)   ; FMADD: 3*2+1
  f4  = 0x40a00000 (5.000000)   ; FMSUB: 3*2-1
  f5  = 0xc0a00000 (-5.000000)  ; FNMSUB: -(3*2-1)
  f6  = 0xc0e00000 (-7.000000)  ; FNMADD: -(3*2+1)
```

## 调试

启用调试模式编译，将打印每条执行的指令：

```bash
make debug
./rv_sim
```

## 许可证

MIT License
