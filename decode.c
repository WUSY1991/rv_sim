/*
 * decode.c - 译码模块
 * 负责解析指令格式，提取操作码、寄存器、立即数等字段
 */

#include "cpu.h"
#include <stdio.h>

/**
 * cpu_decode - 指令译码
 * @cpu: CPU 状态结构体指针 (保留用于未来扩展)
 * @instr: 32 位指令
 * @r: R 型指令结构体指针 (输出)
 * @i: I 型指令结构体指针 (输出)
 * @s: S 型指令结构体指针 (输出)
 * @r4: R4 型指令结构体指针 (输出，用于 F 扩展融合乘加)
 * 
 * 返回：0 表示成功，-1 表示未知指令格式
 * 
 * 说明：该函数将原始指令按不同格式解析，调用者根据 opcode 选择使用哪个结构体
 */
int cpu_decode(CPU *cpu, uint32_t instr, RType *r, IType *i, SType *s, R4Type *r4) {
    (void)cpu;  /* 保留参数用于未来扩展 */
    /* R 型：OP, OP-ATOM, OP-FP, FMADD, FMSUB, FNMSUB, FNMADD */
    r->opcode = instr & 0x7F;
    r->rd     = (instr >> 7) & 0x1F;
    r->funct3 = (instr >> 12) & 0x07;
    r->rs1    = (instr >> 15) & 0x1F;
    r->rs2    = (instr >> 20) & 0x1F;
    r->funct7 = (instr >> 25) & 0x7F;
    
    /* I 型：LOAD, OP-IMM, JALR, SYSTEM */
    i->opcode = instr & 0x7F;
    i->rd     = (instr >> 7) & 0x1F;
    i->funct3 = (instr >> 12) & 0x07;
    i->rs1    = (instr >> 15) & 0x1F;
    i->imm    = sign_extend(instr >> 20, 12);
    
    /* S 型：STORE */
    s->opcode   = instr & 0x7F;
    s->imm_low  = (instr >> 7) & 0x1F;
    s->funct3   = (instr >> 12) & 0x07;
    s->rs1      = (instr >> 15) & 0x1F;
    s->rs2      = (instr >> 20) & 0x1F;
    s->imm_high = (instr >> 25) & 0x7F;
    
    /* R4 型：F 扩展融合乘加指令 (FMADD, FMSUB, FNMSUB, FNMADD) */
    if (r4) {
        r4->opcode = instr & 0x7F;
        r4->rd     = (instr >> 7) & 0x1F;
        r4->rm     = (instr >> 12) & 0x07;   /* 舍入模式 */
        r4->rs1    = (instr >> 15) & 0x1F;
        r4->rs2    = (instr >> 20) & 0x1F;
        r4->fmt    = (instr >> 25) & 0x03;   /* 浮点格式 (00=单精度) */
        r4->rs3    = (instr >> 27) & 0x1F;   /* 第四个操作数 */
    }
    
#ifdef DEBUG_DECODE
    printf("[DECODE] INSTR=0x%08x, opcode=0x%02x, rd=%d, rs1=%d, rs2=%d\n",
           instr, r->opcode, r->rd, r->rs1, r->rs2);
#endif
    
    return 0;
}

/**
 * decode_branch_imm - 解析 B 型指令立即数
 * @instr: 32 位指令
 * 
 * 返回：符号扩展后的分支偏移量
 * 
 * B 型立即数格式：[31|30:25|11:8|7] -> [12|10:5|4:1|11]
 */
int32_t decode_branch_imm(uint32_t instr) {
    uint32_t imm = 0;
    
    imm |= ((instr >> 31) & 0x01) << 12;   /* imm[12] = instr[31] */
    imm |= ((instr >> 7) & 0x01) << 11;    /* imm[11] = instr[7] */
    imm |= ((instr >> 25) & 0x3F) << 5;    /* imm[10:5] = instr[30:25] */
    imm |= ((instr >> 8) & 0x0F) << 1;     /* imm[4:1] = instr[11:8] */
    
    return sign_extend(imm, 13);
}

/**
 * decode_jal_imm - 解析 J 型指令立即数
 * @instr: 32 位指令
 * 
 * 返回：符号扩展后的跳转偏移量
 * 
 * J 型立即数格式：[31|30:21|20|19:12] -> [20|10:1|11|19:12]
 */
int32_t decode_jal_imm(uint32_t instr) {
    uint32_t imm = 0;
    
    imm |= ((instr >> 31) & 0x01) << 20;   /* imm[20] = instr[31] */
    imm |= ((instr >> 21) & 0x3FF) << 1;   /* imm[10:1] = instr[30:21] */
    imm |= ((instr >> 20) & 0x01) << 11;   /* imm[11] = instr[20] */
    imm |= ((instr >> 12) & 0xFF) << 12;   /* imm[19:12] = instr[19:12] */
    
    return sign_extend(imm, 21);
}

/**
 * decode_store_imm - 解析 S 型指令立即数
 * @instr: 32 位指令
 * 
 * 返回：符号扩展后的存储偏移量
 */
int32_t decode_store_imm(uint32_t instr) {
    uint32_t imm = 0;
    
    imm |= ((instr >> 25) & 0x7F) << 5;    /* imm[11:5] = instr[30:25] */
    imm |= ((instr >> 7) & 0x1F);          /* imm[4:0] = instr[11:7] */
    
    return sign_extend(imm, 12);
}
