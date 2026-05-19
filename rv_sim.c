/*
 * RV_sim - 纯 C 语言 RISC-V 模拟器
 * 支持 RV32IMACF 完整指令集
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define MEM_SIZE (1024 * 1024)
#define NUM_REGS 32
#define NUM_FREGS 32

typedef union {
    float f;
    uint32_t u;
} Float32;

typedef struct {
    uint32_t regs[NUM_REGS];
    Float32 fregs[NUM_FREGS];
    uint32_t pc;
    uint32_t memory[MEM_SIZE / 4];
    uint32_t fcsr;
    int has_reservation;
    uint32_t reservation_addr;
} RV32;

typedef struct { uint32_t opcode:7, rd:5, funct3:3, rs1:5, rs2:5, funct7:7; } RType;
typedef struct { uint32_t opcode:7, rd:5, funct3:3, rs1:5, imm:12; } IType;
typedef struct { uint32_t opcode:7, imm_low:5, funct3:3, rs1:5, rs2:5, imm_high:7; } SType;

// M 扩展 funct3
#define F3_MUL    0x0
#define F3_MULH   0x1
#define F3_MULHSU 0x2
#define F3_MULHU  0x3
#define F3_DIV    0x4
#define F3_DIVU   0x5
#define F3_REM    0x6
#define F3_REMU   0x7

// F 扩展 opcode
#define OP_LOAD_FP  0x07
#define OP_STORE_FP 0x27
#define OP_FMADD    0x43
#define OP_FMSUB    0x47
#define OP_FNMSUB   0x4B
#define OP_FNMADD   0x4F
#define OP_OP_FP    0x53

void rv_init(RV32 *rv) {
    memset(rv->regs, 0, sizeof(rv->regs));
    memset(rv->fregs, 0, sizeof(rv->fregs));
    memset(rv->memory, 0, sizeof(rv->memory));
    rv->pc = 0;
    rv->regs[0] = 0;
    rv->fcsr = 0;
    rv->has_reservation = 0;
}

uint32_t rv_fetch(RV32 *rv) {
    if (rv->pc >= MEM_SIZE) return 0;
    return rv->memory[rv->pc / 4];
}

int32_t sign_extend(uint32_t val, int bits) {
    if (val & (1u << (bits - 1))) return val | (~0u << bits);
    return val;
}

// M 扩展
void exec_mul(RV32 *rv, RType *r) {
    rv->regs[r->rd] = (int32_t)rv->regs[r->rs1] * (int32_t)rv->regs[r->rs2];
}

void exec_mulh(RV32 *rv, RType *r) {
    int64_t result = (int64_t)(int32_t)rv->regs[r->rs1] * (int64_t)(int32_t)rv->regs[r->rs2];
    rv->regs[r->rd] = (uint32_t)(result >> 32);
}

void exec_mulhsu(RV32 *rv, RType *r) {
    int64_t result = (int64_t)(int32_t)rv->regs[r->rs1] * (uint64_t)rv->regs[r->rs2];
    rv->regs[r->rd] = (uint32_t)(result >> 32);
}

void exec_mulhu(RV32 *rv, RType *r) {
    uint64_t result = (uint64_t)rv->regs[r->rs1] * (uint64_t)rv->regs[r->rs2];
    rv->regs[r->rd] = (uint32_t)(result >> 32);
}

void exec_div(RV32 *rv, RType *r) {
    int32_t rs1 = (int32_t)rv->regs[r->rs1];
    int32_t rs2 = (int32_t)rv->regs[r->rs2];
    if (rs2 == 0) rv->regs[r->rd] = 0xFFFFFFFF;
    else if (rs1 == (int32_t)0x80000000 && rs2 == -1) rv->regs[r->rd] = 0x80000000;
    else rv->regs[r->rd] = (uint32_t)(rs1 / rs2);
}

void exec_divu(RV32 *rv, RType *r) {
    if (rv->regs[r->rs2] == 0) rv->regs[r->rd] = 0xFFFFFFFF;
    else rv->regs[r->rd] = rv->regs[r->rs1] / rv->regs[r->rs2];
}

void exec_rem(RV32 *rv, RType *r) {
    int32_t rs1 = (int32_t)rv->regs[r->rs1];
    int32_t rs2 = (int32_t)rv->regs[r->rs2];
    if (rs2 == 0) rv->regs[r->rd] = rv->regs[r->rs1];
    else if (rs1 == (int32_t)0x80000000 && rs2 == -1) rv->regs[r->rd] = 0;
    else rv->regs[r->rd] = (uint32_t)(rs1 % rs2);
}

void exec_remu(RV32 *rv, RType *r) {
    if (rv->regs[r->rs2] == 0) rv->regs[r->rd] = rv->regs[r->rs1];
    else rv->regs[r->rd] = rv->regs[r->rs1] % rv->regs[r->rs2];
}

// A 扩展
void exec_lr_w(RV32 *rv, RType *r) {
    uint32_t addr = rv->regs[r->rs1];
    if (addr >= MEM_SIZE) return;
    rv->regs[r->rd] = rv->memory[addr / 4];
    rv->has_reservation = 1;
    rv->reservation_addr = addr;
}

void exec_sc_w(RV32 *rv, RType *r) {
    uint32_t addr = rv->regs[r->rs1];
    if (addr >= MEM_SIZE) { rv->regs[r->rd] = 1; return; }
    if (rv->has_reservation && rv->reservation_addr == addr) {
        rv->memory[addr / 4] = rv->regs[r->rs2];
        rv->regs[r->rd] = 0;
    } else {
        rv->regs[r->rd] = 1;
    }
    rv->has_reservation = 0;
}

void exec_amo(RV32 *rv, RType *r, uint32_t funct5) {
    uint32_t addr = rv->regs[r->rs1];
    if (addr >= MEM_SIZE) return;
    uint32_t *mem_loc = &rv->memory[addr / 4];
    uint32_t old_val = *mem_loc;
    uint32_t new_val;
    
    switch (funct5) {
        case 0x01: new_val = rv->regs[r->rs2]; break;  // AMOSWAP
        case 0x00: new_val = old_val + rv->regs[r->rs2]; break;  // AMOADD
        case 0x04: new_val = old_val ^ rv->regs[r->rs2]; break;  // AMOXOR
        case 0x0C: new_val = old_val & rv->regs[r->rs2]; break;  // AMOAND
        case 0x08: new_val = old_val | rv->regs[r->rs2]; break;  // AMOOR
        case 0x10: new_val = ((int32_t)old_val < (int32_t)rv->regs[r->rs2]) ? old_val : rv->regs[r->rs2]; break;
        case 0x14: new_val = ((int32_t)old_val > (int32_t)rv->regs[r->rs2]) ? old_val : rv->regs[r->rs2]; break;
        case 0x18: new_val = (old_val < rv->regs[r->rs2]) ? old_val : rv->regs[r->rs2]; break;
        case 0x1C: new_val = (old_val > rv->regs[r->rs2]) ? old_val : rv->regs[r->rs2]; break;
        default: new_val = old_val; break;
    }
    *mem_loc = new_val;
    rv->regs[r->rd] = old_val;
}

// F 扩展
void exec_fadd(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = rv->fregs[r->rs1].f + rv->fregs[r->rs2].f;
}

void exec_fsub(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = rv->fregs[r->rs1].f - rv->fregs[r->rs2].f;
}

void exec_fmul(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = rv->fregs[r->rs1].f * rv->fregs[r->rs2].f;
}

void exec_fdiv(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = rv->fregs[r->rs1].f / rv->fregs[r->rs2].f;
}

void exec_fsqrt(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = sqrtf(rv->fregs[r->rs1].f);
}

void exec_fsgnj(RV32 *rv, RType *r, int neg, int xnor) {
    uint32_t sign = (rv->fregs[r->rs2].u >> 31) & 1;
    if (neg) sign = !sign;
    if (xnor) sign = ((rv->fregs[r->rs1].u >> 31) & 1) ^ sign;
    rv->fregs[r->rd].u = (rv->fregs[r->rs1].u & 0x7FFFFFFF) | (sign << 31);
}

void exec_fmin(RV32 *rv, RType *r) {
    float a = rv->fregs[r->rs1].f, b = rv->fregs[r->rs2].f;
    rv->fregs[r->rd].f = (a < b || (a == b && (rv->fregs[r->rs1].u & 0x80000000))) ? a : b;
}

void exec_fmax(RV32 *rv, RType *r) {
    float a = rv->fregs[r->rs1].f, b = rv->fregs[r->rs2].f;
    rv->fregs[r->rd].f = (a > b || (a == b && (rv->fregs[r->rs1].u & 0x80000000))) ? a : b;
}

void exec_fcvt_w_s(RV32 *rv, RType *r, int unsign) {
    if (unsign) rv->regs[r->rd] = (uint32_t)rv->fregs[r->rs1].f;
    else rv->regs[r->rd] = (int32_t)rv->fregs[r->rs1].f;
}

void exec_fcvt_s_w(RV32 *rv, RType *r, int unsign) {
    if (unsign) rv->fregs[r->rd].f = (float)rv->regs[r->rs1];
    else rv->fregs[r->rd].f = (float)(int32_t)rv->regs[r->rs1];
}

void exec_fclass(RV32 *rv, RType *r) {
    uint32_t u = rv->fregs[r->rs1].u;
    uint32_t sign = (u >> 31) & 1;
    uint32_t exp = (u >> 23) & 0xFF;
    uint32_t frac = u & 0x7FFFFF;
    uint32_t result = 0;
    
    if (exp == 0xFF && frac) result = sign ? (1<<0) : (1<<1);
    else if (exp == 0xFF) result = sign ? (1<<2) : (1<<3);
    else if (exp == 0 && frac) result = sign ? (1<<4) : (1<<5);
    else if (exp == 0) result = sign ? (1<<6) : (1<<7);
    else result = sign ? (1<<8) : (1<<9);
    
    rv->regs[r->rd] = result;
}

void exec_fle(RV32 *rv, RType *r) { rv->regs[r->rd] = (rv->fregs[r->rs1].f <= rv->fregs[r->rs2].f) ? 1 : 0; }
void exec_flt(RV32 *rv, RType *r) { rv->regs[r->rd] = (rv->fregs[r->rs1].f < rv->fregs[r->rs2].f) ? 1 : 0; }
void exec_feq(RV32 *rv, RType *r) { rv->regs[r->rd] = (rv->fregs[r->rs1].f == rv->fregs[r->rs2].f) ? 1 : 0; }

// F 扩展融合乘加 (rs2 字段实际是 rs3)
void exec_fmadd(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = rv->fregs[r->rs1].f * rv->fregs[r->rs2].f + rv->fregs[(r->funct7 >> 2) & 0x1F].f;
}

void exec_fmsub(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = rv->fregs[r->rs1].f * rv->fregs[r->rs2].f - rv->fregs[(r->funct7 >> 2) & 0x1F].f;
}

void exec_fnmsub(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = -(rv->fregs[r->rs1].f * rv->fregs[r->rs2].f - rv->fregs[(r->funct7 >> 2) & 0x1F].f);
}

void exec_fnmadd(RV32 *rv, RType *r) {
    rv->fregs[r->rd].f = -(rv->fregs[r->rs1].f * rv->fregs[r->rs2].f + rv->fregs[(r->funct7 >> 2) & 0x1F].f);
}

int rv_execute(RV32 *rv, uint32_t instr) {
    RType *r = (RType *)&instr;
    IType *i = (IType *)&instr;
    SType *s = (SType *)&instr;
    
    uint32_t opcode = r->opcode;
    uint32_t funct3 = r->funct3;
    uint32_t funct7 = r->funct7;

    switch (opcode) {
        case 0x33:  // OP (M 扩展和基础 OP)
            if (funct3 >= F3_MUL && funct3 <= F3_REMU) {
                switch (funct3) {
                    case F3_MUL: exec_mul(rv, r); break;
                    case F3_MULH: exec_mulh(rv, r); break;
                    case F3_MULHSU: exec_mulhsu(rv, r); break;
                    case F3_MULHU: exec_mulhu(rv, r); break;
                    case F3_DIV: exec_div(rv, r); break;
                    case F3_DIVU: exec_divu(rv, r); break;
                    case F3_REM: exec_rem(rv, r); break;
                    case F3_REMU: exec_remu(rv, r); break;
                }
                rv->pc += 4;
                break;
            }
            // 基础 OP
            if (funct7 == 0x00 || funct7 == 0x20) {
                switch (funct3) {
                    case 0x0: rv->regs[r->rd] = (funct7 == 0x20) ? rv->regs[r->rs1] - rv->regs[r->rs2] : rv->regs[r->rs1] + rv->regs[r->rs2]; break;
                    case 0x1: rv->regs[r->rd] = rv->regs[r->rs1] << (rv->regs[r->rs2] & 0x1F); break;
                    case 0x2: rv->regs[r->rd] = ((int32_t)rv->regs[r->rs1] < (int32_t)rv->regs[r->rs2]) ? 1 : 0; break;
                    case 0x3: rv->regs[r->rd] = (rv->regs[r->rs1] < rv->regs[r->rs2]) ? 1 : 0; break;
                    case 0x4: rv->regs[r->rd] = rv->regs[r->rs1] ^ rv->regs[r->rs2]; break;
                    case 0x5: rv->regs[r->rd] = (funct7 == 0x20) ? ((int32_t)rv->regs[r->rs1] >> (rv->regs[r->rs2] & 0x1F)) : (rv->regs[r->rs1] >> (rv->regs[r->rs2] & 0x1F)); break;
                    case 0x6: rv->regs[r->rd] = rv->regs[r->rs1] | rv->regs[r->rs2]; break;
                    case 0x7: rv->regs[r->rd] = rv->regs[r->rs1] & rv->regs[r->rs2]; break;
                }
            }
            rv->pc += 4;
            break;
            
        case 0x2F:  // OP-ATOM (A 扩展)
            {
                uint32_t funct5 = funct7 >> 2;
                if (funct3 == 0x02 && funct5 == 0x02) exec_lr_w(rv, r);
                else if (funct3 == 0x02 && funct5 == 0x03) exec_sc_w(rv, r);
                else exec_amo(rv, r, funct5);
            }
            rv->pc += 4;
            break;
            
        case OP_LOAD_FP:
            {
                uint32_t addr = rv->regs[i->rs1] + i->imm;
                if (addr < MEM_SIZE) rv->fregs[i->rd].u = rv->memory[addr / 4];
            }
            rv->pc += 4;
            break;
            
        case OP_STORE_FP:
            {
                int32_t imm = (s->imm_high << 5) | s->imm_low;
                uint32_t addr = rv->regs[s->rs1] + imm;
                if (addr < MEM_SIZE) rv->memory[addr / 4] = rv->fregs[s->rs2].u;
            }
            rv->pc += 4;
            break;
            
        case OP_FMADD:  // FMADD.S
            exec_fmadd(rv, r);
            rv->pc += 4;
            break;
        case OP_FMSUB:  // FMSUB.S
            exec_fmsub(rv, r);
            rv->pc += 4;
            break;
        case OP_FNMSUB:  // FNMSUB.S
            exec_fnmsub(rv, r);
            rv->pc += 4;
            break;
        case OP_FNMADD:  // FNMADD.S
            exec_fnmadd(rv, r);
            rv->pc += 4;
            break;
        case OP_OP_FP:
            {
                uint32_t fmt = (funct7 >> 2) & 0x7;
                uint32_t rm = r->rs2;
                if (fmt == 0) {  // 单精度
                    switch (funct7 & 0x60) {
                        case 0x00: exec_fadd(rv, r); break;
                        case 0x04: exec_fsub(rv, r); break;
                        case 0x08: exec_fmul(rv, r); break;
                        case 0x0C: exec_fdiv(rv, r); break;
                        case 0x14: exec_fsqrt(rv, r); break;
                        case 0x10:
                            if (rm == 0) exec_fsgnj(rv, r, 0, 0);
                            else if (rm == 1) exec_fsgnj(rv, r, 1, 0);
                            else if (rm == 2) exec_fsgnj(rv, r, 0, 1);
                            break;
                        case 0x18:
                            if (rm == 0) exec_fmin(rv, r);
                            else if (rm == 1) exec_fmax(rv, r);
                            break;
                        case 0x20:
                            if (rm == 0) exec_fcvt_w_s(rv, r, 0);
                            else if (rm == 1) exec_fcvt_w_s(rv, r, 1);
                            break;
                        case 0x60:
                            if (rm == 0) exec_fcvt_s_w(rv, r, 0);
                            else if (rm == 1) exec_fcvt_s_w(rv, r, 1);
                            break;
                        case 0x50:
                            if (rm == 1) exec_feq(rv, r);
                            else if (rm == 2) exec_flt(rv, r);
                            else if (rm == 3) exec_fle(rv, r);
                            break;
                        case 0x58: if (rm == 0) exec_fclass(rv, r); break;
                    }
                }
            }
            rv->pc += 4;
            break;
            
        case 0x37: rv->regs[r->rd] = instr & 0xFFFFF000; rv->pc += 4; break;  // LUI
        case 0x17: rv->regs[r->rd] = rv->pc + (instr & 0xFFFFF000); rv->pc += 4; break;  // AUIPC
        case 0x6F:  // JAL
            {
                int32_t imm = ((instr >> 31) & 1) ? 0xFF000000 : 0;
                imm |= (instr >> 20) & 0xFF000;
                imm |= (instr >> 11) & 0x1000;
                imm |= (instr >> 21) & 0xFF0000;
                rv->regs[r->rd] = rv->pc + 4;
                rv->pc += imm;
            }
            break;
        case 0x67:  // JALR
            {
                uint32_t target = (rv->regs[i->rs1] + i->imm) & ~1;
                rv->regs[i->rd] = rv->pc + 4;
                rv->pc = target;
            }
            break;
        case 0x63:  // BRANCH
            {
                int32_t imm = ((instr >> 31) & 1) ? 0xFFFFF000 : 0;
                imm |= ((instr >> 7) & 0x1E) | ((instr >> 25) & 0x7E0) | ((instr >> 8) & 0x1000);
                int32_t rs1 = (int32_t)rv->regs[s->rs1], rs2 = (int32_t)rv->regs[s->rs2];
                int taken = 0;
                switch (funct3) {
                    case 0x0: taken = (rs1 == rs2); break;
                    case 0x1: taken = (rs1 != rs2); break;
                    case 0x4: taken = (rs1 < rs2); break;
                    case 0x5: taken = (rs1 >= rs2); break;
                    case 0x6: taken = ((uint32_t)rs1 < (uint32_t)rs2); break;
                    case 0x7: taken = ((uint32_t)rs1 >= (uint32_t)rs2); break;
                }
                rv->pc += taken ? imm : 4;
            }
            break;
        case 0x03:  // LOAD
            {
                uint32_t addr = rv->regs[i->rs1] + i->imm;
                if (addr < MEM_SIZE) {
                    switch (funct3) {
                        case 0x0: rv->regs[i->rd] = sign_extend(rv->memory[addr / 4] >> ((addr % 4) * 8), 8); break;
                        case 0x1: rv->regs[i->rd] = sign_extend(rv->memory[addr / 4] >> ((addr % 4) * 8), 16); break;
                        case 0x2: rv->regs[i->rd] = rv->memory[addr / 4]; break;
                    }
                }
            }
            rv->pc += 4;
            break;
        case 0x23:  // STORE
            {
                int32_t imm = ((instr >> 25) << 5) | ((instr >> 7) & 0x1F);
                uint32_t addr = rv->regs[s->rs1] + imm;
                if (addr < MEM_SIZE) {
                    switch (funct3) {
                        case 0x0: rv->memory[addr / 4] &= ~(0xFF << ((addr % 4) * 8)); rv->memory[addr / 4] |= (rv->regs[s->rs2] & 0xFF) << ((addr % 4) * 8); break;
                        case 0x1: rv->memory[addr / 4] &= ~(0xFFFF << ((addr % 4) * 8)); rv->memory[addr / 4] |= (rv->regs[s->rs2] & 0xFFFF) << ((addr % 4) * 8); break;
                        case 0x2: rv->memory[addr / 4] = rv->regs[s->rs2]; break;
                    }
                }
            }
            rv->pc += 4;
            break;
        case 0x13:  // OP-IMM
            {
                int32_t imm = i->imm;
                switch (funct3) {
                    case 0x0: rv->regs[i->rd] = rv->regs[i->rs1] + imm; break;
                    case 0x1: rv->regs[i->rd] = rv->regs[i->rs1] << (imm & 0x1F); break;
                    case 0x2: rv->regs[i->rd] = ((int32_t)rv->regs[i->rs1] < imm) ? 1 : 0; break;
                    case 0x3: rv->regs[i->rd] = (rv->regs[i->rs1] < (uint32_t)imm) ? 1 : 0; break;
                    case 0x4: rv->regs[i->rd] = rv->regs[i->rs1] ^ imm; break;
                    case 0x5: rv->regs[i->rd] = (imm & 0x400) ? ((int32_t)rv->regs[i->rs1] >> (imm & 0x1F)) : (rv->regs[i->rs1] >> (imm & 0x1F)); break;
                    case 0x6: rv->regs[i->rd] = rv->regs[i->rs1] | imm; break;
                    case 0x7: rv->regs[i->rd] = rv->regs[i->rs1] & imm; break;
                }
            }
            rv->pc += 4;
            break;
        case 0x73:  // SYSTEM
            if ((instr & 0xFE000FFF) == 0x00000073) { printf("\n[ECALL]\n"); return 0; }
            rv->pc += 4;
            break;
        default:
            fprintf(stderr, "未知指令：0x%08x\n", instr);
            return -1;
    }
    rv->regs[0] = 0;
    return 1;
}

void rv_run(RV32 *rv, int max_cycles) {
    int cycles = 0;
    printf("开始执行 (PC=0x%08x)...\n\n", rv->pc);
    while (cycles < max_cycles) {
        uint32_t instr = rv_fetch(rv);
        if (instr == 0) break;
        printf("[0x%08x] 0x%08x\n", rv->pc, instr);
        if (rv_execute(rv, instr) <= 0) break;
        cycles++;
    }
    printf("\n=== 完成 (%d 周期) ===\n", cycles);
    printf("\n通用寄存器:\n");
    for (int i = 0; i < 32; i++) if (rv->regs[i]) printf("  x%-2d = 0x%08x (%d)\n", i, rv->regs[i], (int32_t)rv->regs[i]);
    printf("\n浮点寄存器:\n");
    for (int i = 0; i < 32; i++) if (rv->fregs[i].u) printf("  f%-2d = 0x%08x (%.6f)\n", i, rv->fregs[i].u, rv->fregs[i].f);
}

uint32_t test_program[] = {
    // M 扩展测试
    0x00500113,  // addi x2, x0, 5
    0x00700193,  // addi x3, x0, 7
    0x023100B3,  // mul  x1, x2, x3  (35)
    0x02314133,  // div  x2, x2, x3  (0)
    0x023161B3,  // rem  x3, x2, x3  (0)
    // F 扩展测试
    0x00300113,  // addi x2, x0, 3
    0x00200193,  // addi x3, x0, 2
    0x00100213,  // addi x4, x0, 1
    0xc0010053,  // fcvt.s.w f0, x2 (3.0)
    0xc00180d3,  // fcvt.s.w f1, x3 (2.0)
    0xc0020153,  // fcvt.s.w f2, x4 (1.0)
    0x101001c3,  // fmadd.s f3, f0, f1, f2 (7.0)
    0x10100247,  // fmsub.s f4, f0, f1, f2 (5.0)
    0x101002cb,  // fnmsub.s f5, f0, f1, f2 (-5.0)
    0x1010034f,  // fnmadd.s f6, f0, f1, f2 (-7.0)
    0x00000073,  // ecall
};

int main(int argc, char *argv[]) {
    RV32 rv;
    rv_init(&rv);
    if (argc > 1 && strcmp(argv[1], "-h") == 0) {
        printf("RV_sim - RISC-V RV32IMACF\n用法：./rv_sim [file.bin]\n");
        return 0;
    }
    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        if (!f) { fprintf(stderr, "无法打开：%s\n", argv[1]); return 1; }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint32_t *prog = malloc(size);
        fread(prog, 1, size, f);
        fclose(f);
        for (int i = 0; i < size/4; i++) rv.memory[i] = prog[i];
        free(prog);
        rv_run(&rv, 1000);
    } else {
        for (int i = 0; i < sizeof(test_program)/4; i++) rv.memory[i] = test_program[i];
        rv_run(&rv, 1000);
    }
    return 0;
}
