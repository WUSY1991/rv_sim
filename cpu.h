/*
 * cpu.h - RISC-V CPU 核心结构体定义
 * 支持 RV32IMACF 完整指令集
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEM_SIZE (1024 * 1024)
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

/* CPU 状态结构体 - 包含所有寄存器和状态 */
typedef struct {
    /* 通用寄存器 x0-x31 */
    uint32_t regs[NUM_REGS];
    
    /* 浮点寄存器 f0-f31 */
    Float32 fregs[NUM_FREGS];
    
    /* 程序计数器 */
    uint32_t pc;
    
    /* 内存 (字访问) */
    uint32_t memory[MEM_SIZE / 4];
    
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
uint32_t cpu_fetch(CPU *cpu);
int cpu_decode(CPU *cpu, uint32_t instr, RType *r, IType *i, SType *s, R4Type *r4);
int cpu_execute(CPU *cpu, uint32_t instr);
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
