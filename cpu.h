/*
 * cpu.h - RISC-V CPU 核心结构体定义
 * 支持 RV32IMACF 完整指令集
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "config.h"
#include "mem.h"  /* 内存配置由 mem.h 提供 */

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
    uint32_t opcode:7, rd:5, funct3:3, rs1:5;
    int32_t imm;  /* 立即数需要符号扩展，不能用12位位域 */
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
typedef struct CPU {
    union {
        uint32_t regs[NUM_REGS];  /* 数组访问：cpu->regs[idx] */
        struct {
            uint32_t zero;        /* x0 - 硬连线为 0 */
            uint32_t ra;          /* x1 - 返回地址 */
            uint32_t sp;          /* x2 - 栈指针 */
            uint32_t gp;          /* x3 - 全局指针 */
            uint32_t tp;          /* x4 - 线程指针 */
            uint32_t t0, t1, t2;  /* x5-x7 - 临时寄存器 */
            uint32_t s0, s1;      /* x8-x9 - 保存寄存器 */
            uint32_t a0, a1, a2, a3, a4, a5, a6, a7;  /* x10-x17 - 参数寄存器 */
            uint32_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;  /* x18-x27 */
            uint32_t t3, t4, t5, t6;  /* x28-x31 - 临时寄存器 */
        };  /* 别名访问：cpu->sp, cpu->ra 等 */
    };  /* 匿名 union，成员可直接访问 */
    union {
        /* 浮点寄存器 f0-f31 */
        Float32 fregs[NUM_FREGS];
        struct {
            Float32 ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7;
            Float32 fs0, fs1;
            Float32 fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7;
            Float32 fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11;
            Float32 ft8, ft9, ft10, ft11, ft12, ft13, ft14, ft15;
        };
    };
    /* 程序计数器 */
    uint32_t pc;

    /* 指令内存 (IMEM) */
    uint32_t imem[IMEM_SIZE / 4];

    /* 数据内存 (DMEM) */
    uint32_t dmem[DMEM_SIZE / 4];

    uint32_t breakpoint[4];
    uint32_t brkctrl[4];

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
