/*
 * cpu.h - RISC-V CPU 核心结构体定义
 * 支持 RV32IMACF 完整指令集
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

/* ========== 可配置参数 ========== */
/* 指令内存 (IMEM) */
#define IMEM_BASE_ADDR  0x1000000   /* 指令内存基地址 */
#define IMEM_SIZE       (256 * 1024)  /* 指令内存大小 256KB */

/* 数据内存 (DMEM) */
#define DMEM_BASE_ADDR  0x20000     /* 数据内存基地址 */
#define DMEM_SIZE       (256 * 1024)  /* 数据内存大小 256KB */

/* PC 复位地址 */
#define PC_RESET_ADDR   0x1000080   /* PC 复位地址 */

/* ========== UART 虚拟串口 ========== */
#define UART_TX_ADDR    0x8200      /* 发送寄存器地址 */
#define UART_RX_ADDR    0x8210      /* 接收寄存器地址 */

#define NUM_REGS 32
#define NUM_FREGS 32

/* 浮点数联合体 */
typedef union {
    float f;
    uint32_t u;
} Float32;

/* 指令类型定义 */
typedef struct {
    uint32_t opcode:7, rd:5, funct3:3, rs1:5, rs2:5, funct7:7;
} RType;

typedef struct {
    uint32_t opcode:7, rd:5, funct3:3, rs1:5, imm:12;
} IType;

typedef struct {
    uint32_t opcode:7, imm_low:5, funct3:3, rs1:5, rs2:5, imm_high:7;
} SType;

typedef struct {
    uint32_t opcode:7, rd:5, rm:3, rs1:5, rs2:5, fmt:2, rs3:5;
} R4Type;  /* F 扩展融合乘加指令 */

/* 压缩指令 quadrant 定义 */
#define C_QUADRANT_0  0x00  /* C.ADDI4SPN, C.LW, C.SW */
#define C_QUADRANT_1  0x01  /* C.ADDI, C.JAL, C.LI, C.J, C.BEQZ */
#define C_QUADRANT_2  0x02  /* C.SLLI, C.LWSP, C.JR, C.MV, C.ADD */
#define C_QUADRANT_3  0x03  /* 32 位标准指令 */

/* CPU 状态结构体 - 包含所有寄存器和状态 */
typedef struct {
    /* 通用寄存器 x0-x31 */
    uint32_t regs[NUM_REGS];

    /* 浮点寄存器 f0-f31 */
    Float32 fregs[NUM_FREGS];

    /* 程序计数器 */
    uint32_t pc;

    /* 指令内存 (IMEM) */
    uint32_t imem[IMEM_SIZE / 4];

    /* 数据内存 (DMEM) */
    uint32_t dmem[DMEM_SIZE / 4];

    /* UART 虚拟串口状态 */
    uint8_t uart_tx_char;       /* 待发送字符 */
    uint8_t uart_rx_char;       /* 接收到的字符 */
    uint8_t uart_rx_valid;      /* 接收有效标志 */
    uint8_t uart_tx_ready;      /* 发送就绪标志 (始终为1) */

    /* 浮点控制状态寄存器 */
    uint32_t fcsr;

    /* 原子操作保留地址 */
    int has_reservation;
    uint32_t reservation_addr;

    /* 运行状态 */
    int halted;
    uint32_t cycles;
} CPU;

/* 指令操作码定义 */
#define OP_LOAD     0x03
#define OP_STORE    0x23
#define OP_OP_IMM   0x13
#define OP_OP       0x33
#define OP_LUI      0x37
#define OP_AUIPC    0x17
#define OP_BRANCH   0x63
#define OP_JALR     0x67
#define OP_JAL      0x6F
#define OP_SYSTEM   0x73

/* M 扩展 funct3 */
#define F3_MUL    0x0
#define F3_MULH   0x1
#define F3_MULHSU 0x2
#define F3_MULHU  0x3
#define F3_DIV    0x4
#define F3_DIVU   0x5
#define F3_REM    0x6
#define F3_REMU   0x7

/* A 扩展 funct3 */
#define F3_AMO    0x02

/* CSR 地址定义 */
#define CSR_FFLAGS  0x001  /* 浮点异常标志 */
#define CSR_FRM     0x002  /* 浮点舍入模式 */
#define CSR_FCSR    0x003  /* 浮点控制状态寄存器 (FCSR = FRM<<5 | FFLAGS) */

/* CSR 指令 funct3 */
#define F3_CSRRW   0x1
#define F3_CSRRS   0x2
#define F3_CSRRC   0x3
#define F3_CSRRWI  0x5
#define F3_CSRRSI  0x6
#define F3_CSRRCI  0x7

/* F 扩展 opcode */
#define OP_LOAD_FP   0x07
#define OP_STORE_FP  0x27
#define OP_FMADD     0x43
#define OP_FMSUB     0x47
#define OP_FNMSUB    0x4B
#define OP_FNMADD    0x4F
#define OP_OP_FP     0x53

/* 函数声明 */
void cpu_init(CPU *cpu);
int is_compressed(uint16_t instr_low);
uint32_t cpu_fetch(CPU *cpu, int *instr_len);
uint32_t expand_compressed(uint16_t instr);
int cpu_decode(CPU *cpu, uint32_t instr, RType *r, IType *i, SType *s, R4Type *r4);
int cpu_execute(CPU *cpu, uint32_t instr, int instr_len);
void cpu_writeback(CPU *cpu, uint32_t rd, uint32_t value);
void cpu_run(CPU *cpu, int max_cycles);
void cpu_dump(CPU *cpu);

/* 符号扩展辅助函数 */
static inline int32_t sign_extend(uint32_t val, int bits) {
    if (val & (1u << (bits - 1)))
        return val | (~0u << bits);
    return val;
}

#endif /* CPU_H */
