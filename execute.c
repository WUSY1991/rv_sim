/*
 * execute.c - 执行模块
 * 负责执行所有 RV32IMACF 指令
 */

#include "cpu.h"
#include <stdio.h>
#include <math.h>

/* 外部函数声明 */
extern int32_t decode_branch_imm(uint32_t instr);
extern int32_t decode_jal_imm(uint32_t instr);
extern int32_t decode_store_imm(uint32_t instr);

/* ==================== M 扩展 (乘除) ==================== */

static void exec_mul(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (int32_t)cpu->regs[r->rs1] * (int32_t)cpu->regs[r->rs2];
}

static void exec_mulh(CPU *cpu, RType *r) {
    int64_t result = (int64_t)(int32_t)cpu->regs[r->rs1] * (int64_t)(int32_t)cpu->regs[r->rs2];
    cpu->regs[r->rd] = (uint32_t)(result >> 32);
}

static void exec_mulhsu(CPU *cpu, RType *r) {
    int64_t result = (int64_t)(int32_t)cpu->regs[r->rs1] * (uint64_t)cpu->regs[r->rs2];
    cpu->regs[r->rd] = (uint32_t)(result >> 32);
}

static void exec_mulhu(CPU *cpu, RType *r) {
    uint64_t result = (uint64_t)cpu->regs[r->rs1] * (uint64_t)cpu->regs[r->rs2];
    cpu->regs[r->rd] = (uint32_t)(result >> 32);
}

static void exec_div(CPU *cpu, RType *r) {
    int32_t rs1 = (int32_t)cpu->regs[r->rs1];
    int32_t rs2 = (int32_t)cpu->regs[r->rs2];
    if (rs2 == 0)
        cpu->regs[r->rd] = 0xFFFFFFFF;
    else if (rs1 == (int32_t)0x80000000 && rs2 == -1)
        cpu->regs[r->rd] = 0x80000000;
    else
        cpu->regs[r->rd] = (uint32_t)(rs1 / rs2);
}

static void exec_divu(CPU *cpu, RType *r) {
    if (cpu->regs[r->rs2] == 0)
        cpu->regs[r->rd] = 0xFFFFFFFF;
    else
        cpu->regs[r->rd] = cpu->regs[r->rs1] / cpu->regs[r->rs2];
}

static void exec_rem(CPU *cpu, RType *r) {
    int32_t rs1 = (int32_t)cpu->regs[r->rs1];
    int32_t rs2 = (int32_t)cpu->regs[r->rs2];
    if (rs2 == 0)
        cpu->regs[r->rd] = cpu->regs[r->rs1];
    else if (rs1 == (int32_t)0x80000000 && rs2 == -1)
        cpu->regs[r->rd] = 0;
    else
        cpu->regs[r->rd] = (uint32_t)(rs1 % rs2);
}

static void exec_remu(CPU *cpu, RType *r) {
    if (cpu->regs[r->rs2] == 0)
        cpu->regs[r->rd] = cpu->regs[r->rs1];
    else
        cpu->regs[r->rd] = cpu->regs[r->rs1] % cpu->regs[r->rs2];
}

/* ==================== A 扩展 (原子操作) ==================== */

static void exec_lr_w(CPU *cpu, RType *r) {
    uint32_t addr = cpu->regs[r->rs1];
    if (addr >= MEM_SIZE) return;
    cpu->regs[r->rd] = cpu->memory[addr / 4];
    cpu->has_reservation = 1;
    cpu->reservation_addr = addr;
}

static void exec_sc_w(CPU *cpu, RType *r) {
    uint32_t addr = cpu->regs[r->rs1];
    if (addr >= MEM_SIZE) {
        cpu->regs[r->rd] = 1;
        return;
    }
    if (cpu->has_reservation && cpu->reservation_addr == addr) {
        cpu->memory[addr / 4] = cpu->regs[r->rs2];
        cpu->regs[r->rd] = 0;
    } else {
        cpu->regs[r->rd] = 1;
    }
    cpu->has_reservation = 0;
}

static void exec_amo(CPU *cpu, RType *r, uint32_t funct5) {
    uint32_t addr = cpu->regs[r->rs1];
    if (addr >= MEM_SIZE) return;
    
    uint32_t *mem_loc = &cpu->memory[addr / 4];
    uint32_t old_val = *mem_loc;
    uint32_t new_val;
    
    switch (funct5) {
        case 0x01: new_val = cpu->regs[r->rs2]; break;  /* AMOSWAP */
        case 0x00: new_val = old_val + cpu->regs[r->rs2]; break;  /* AMOADD */
        case 0x04: new_val = old_val ^ cpu->regs[r->rs2]; break;  /* AMOXOR */
        case 0x0C: new_val = old_val & cpu->regs[r->rs2]; break;  /* AMOAND */
        case 0x08: new_val = old_val | cpu->regs[r->rs2]; break;  /* AMOOR */
        case 0x10: new_val = ((int32_t)old_val < (int32_t)cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMIN */
        case 0x14: new_val = ((int32_t)old_val > (int32_t)cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMAX */
        case 0x18: new_val = (old_val < cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMINU */
        case 0x1C: new_val = (old_val > cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMAXU */
        default: new_val = old_val; break;
    }
    
    *mem_loc = new_val;
    cpu->regs[r->rd] = old_val;
}

/* ==================== F 扩展 (浮点运算) ==================== */

static void exec_fadd(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = cpu->fregs[r->rs1].f + cpu->fregs[r->rs2].f;
}

static void exec_fsub(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = cpu->fregs[r->rs1].f - cpu->fregs[r->rs2].f;
}

static void exec_fmul(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = cpu->fregs[r->rs1].f * cpu->fregs[r->rs2].f;
}

static void exec_fdiv(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = cpu->fregs[r->rs1].f / cpu->fregs[r->rs2].f;
}

static void exec_fsqrt(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = sqrtf(cpu->fregs[r->rs1].f);
}

static void exec_fsgnj(CPU *cpu, RType *r, int neg, int xnor) {
    uint32_t sign = (cpu->fregs[r->rs2].u >> 31) & 1;
    if (neg) sign = !sign;
    if (xnor) sign = ((cpu->fregs[r->rs1].u >> 31) & 1) ^ sign;
    cpu->fregs[r->rd].u = (cpu->fregs[r->rs1].u & 0x7FFFFFFF) | (sign << 31);
}

static void exec_fmin(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    cpu->fregs[r->rd].f = (a < b || (a == b && (cpu->fregs[r->rs1].u & 0x80000000))) ? a : b;
}

static void exec_fmax(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    cpu->fregs[r->rd].f = (a > b || (a == b && (cpu->fregs[r->rs1].u & 0x80000000))) ? a : b;
}

static void exec_fcvt_w_s(CPU *cpu, RType *r, int unsign) {
    if (unsign)
        cpu->regs[r->rd] = (uint32_t)cpu->fregs[r->rs1].f;
    else
        cpu->regs[r->rd] = (int32_t)cpu->fregs[r->rs1].f;
}

static void exec_fcvt_s_w(CPU *cpu, RType *r, int unsign) {
    if (unsign)
        cpu->fregs[r->rd].f = (float)cpu->regs[r->rs1];
    else
        cpu->fregs[r->rd].f = (float)(int32_t)cpu->regs[r->rs1];
}

static void exec_fclass(CPU *cpu, RType *r) {
    uint32_t u = cpu->fregs[r->rs1].u;
    uint32_t sign = (u >> 31) & 1;
    uint32_t exp = (u >> 23) & 0xFF;
    uint32_t frac = u & 0x7FFFFF;
    uint32_t result = 0;
    
    if (exp == 0xFF && frac)
        result = sign ? (1<<0) : (1<<1);      /* NaN */
    else if (exp == 0xFF)
        result = sign ? (1<<2) : (1<<3);      /* 无穷 */
    else if (exp == 0 && frac)
        result = sign ? (1<<4) : (1<<5);      /* 次正规数 */
    else if (exp == 0)
        result = sign ? (1<<6) : (1<<7);      /* 零 */
    else
        result = sign ? (1<<8) : (1<<9);      /* 正规数 */
    
    cpu->regs[r->rd] = result;
}

static void exec_fle(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (cpu->fregs[r->rs1].f <= cpu->fregs[r->rs2].f) ? 1 : 0;
}

static void exec_flt(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (cpu->fregs[r->rs1].f < cpu->fregs[r->rs2].f) ? 1 : 0;
}

static void exec_feq(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (cpu->fregs[r->rs1].f == cpu->fregs[r->rs2].f) ? 1 : 0;
}

/* F 扩展融合乘加 (rs2 字段实际是 rs3) */
static void exec_fmadd(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = cpu->fregs[r->rs1].f * cpu->fregs[r->rs2].f + cpu->fregs[(r->funct7 >> 2) & 0x1F].f;
}

static void exec_fmsub(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = cpu->fregs[r->rs1].f * cpu->fregs[r->rs2].f - cpu->fregs[(r->funct7 >> 2) & 0x1F].f;
}

static void exec_fnmsub(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = -(cpu->fregs[r->rs1].f * cpu->fregs[r->rs2].f - cpu->fregs[(r->funct7 >> 2) & 0x1F].f);
}

static void exec_fnmadd(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].f = -(cpu->fregs[r->rs1].f * cpu->fregs[r->rs2].f + cpu->fregs[(r->funct7 >> 2) & 0x1F].f);
}

/* ==================== 基础整数运算 ==================== */

static void exec_add(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] + cpu->regs[r->rs2];
}

static void exec_sub(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] - cpu->regs[r->rs2];
}

static void exec_sll(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] << (cpu->regs[r->rs2] & 0x1F);
}

static void exec_slt(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = ((int32_t)cpu->regs[r->rs1] < (int32_t)cpu->regs[r->rs2]) ? 1 : 0;
}

static void exec_sltu(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (cpu->regs[r->rs1] < cpu->regs[r->rs2]) ? 1 : 0;
}

static void exec_xor(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] ^ cpu->regs[r->rs2];
}

static void exec_srl(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] >> (cpu->regs[r->rs2] & 0x1F);
}

static void exec_sra(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (uint32_t)((int32_t)cpu->regs[r->rs1] >> (cpu->regs[r->rs2] & 0x1F));
}

static void exec_or(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] | cpu->regs[r->rs2];
}

static void exec_and(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] & cpu->regs[r->rs2];
}

/* 立即数运算 */
static void exec_addi(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] + i->imm;
}

static void exec_slli(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] << (i->imm & 0x1F);
}

static void exec_sliti(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = ((int32_t)cpu->regs[i->rs1] < i->imm) ? 1 : 0;
}

static void exec_sltiu(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = (cpu->regs[i->rs1] < (uint32_t)i->imm) ? 1 : 0;
}

static void exec_xori(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] ^ i->imm;
}

static void exec_srli(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] >> (i->imm & 0x1F);
}

static void exec_srai(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = (uint32_t)((int32_t)cpu->regs[i->rs1] >> (i->imm & 0x1F));
}

static void exec_ori(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] | i->imm;
}

static void exec_andi(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] & i->imm;
}

/* ==================== 主执行函数 ==================== */

/**
 * cpu_execute - 执行指令
 * @cpu: CPU 状态结构体指针
 * @instr: 32 位指令
 * 
 * 返回：1 表示继续执行，0 表示停止 (ECALL/未知指令)，-1 表示错误
 */
int cpu_execute(CPU *cpu, uint32_t instr) {
    RType r_inst;
    IType i_inst;
    SType s_inst;
    
    cpu_decode(cpu, instr, &r_inst, &i_inst, &s_inst, NULL);
    
    uint32_t opcode = r_inst.opcode;
    uint32_t funct3 = r_inst.funct3;
    uint32_t funct7 = r_inst.funct7;
    
    switch (opcode) {
        /* ==================== OP (0x33) ==================== */
        case OP_OP:
            /* M 扩展 (funct3: 0-7) */
            if (funct3 <= F3_REMU) {
                switch (funct3) {
                    case F3_MUL:   exec_mul(cpu, &r_inst); break;
                    case F3_MULH:  exec_mulh(cpu, &r_inst); break;
                    case F3_MULHSU: exec_mulhsu(cpu, &r_inst); break;
                    case F3_MULHU: exec_mulhu(cpu, &r_inst); break;
                    case F3_DIV:   exec_div(cpu, &r_inst); break;
                    case F3_DIVU:  exec_divu(cpu, &r_inst); break;
                    case F3_REM:   exec_rem(cpu, &r_inst); break;
                    case F3_REMU:  exec_remu(cpu, &r_inst); break;
                }
                cpu->pc += 4;
                break;
            }
            
            /* 基础 OP */
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x20) exec_sub(cpu, &r_inst);
                    else exec_add(cpu, &r_inst);
                    break;
                case 0x1: exec_sll(cpu, &r_inst); break;
                case 0x2: exec_slt(cpu, &r_inst); break;
                case 0x3: exec_sltu(cpu, &r_inst); break;
                case 0x4: exec_xor(cpu, &r_inst); break;
                case 0x5:
                    if (funct7 == 0x20) exec_sra(cpu, &r_inst);
                    else exec_srl(cpu, &r_inst);
                    break;
                case 0x6: exec_or(cpu, &r_inst); break;
                case 0x7: exec_and(cpu, &r_inst); break;
            }
            cpu->pc += 4;
            break;
            
        /* ==================== OP-IMM (0x13) ==================== */
        case OP_OP_IMM:
            switch (funct3) {
                case 0x0: exec_addi(cpu, &i_inst); break;
                case 0x1: exec_slli(cpu, &i_inst); break;
                case 0x2: exec_sliti(cpu, &i_inst); break;
                case 0x3: exec_sltiu(cpu, &i_inst); break;
                case 0x4: exec_xori(cpu, &i_inst); break;
                case 0x5:
                    if (i_inst.imm & 0x400) exec_srai(cpu, &i_inst);
                    else exec_srli(cpu, &i_inst);
                    break;
                case 0x6: exec_ori(cpu, &i_inst); break;
                case 0x7: exec_andi(cpu, &i_inst); break;
            }
            cpu->pc += 4;
            break;
            
        /* ==================== OP-ATOM (0x2F) - A 扩展 ==================== */
        case 0x2F:
            {
                uint32_t funct5 = funct7 >> 2;
                if (funct3 == F3_AMO && funct5 == 0x02)
                    exec_lr_w(cpu, &r_inst);
                else if (funct3 == F3_AMO && funct5 == 0x03)
                    exec_sc_w(cpu, &r_inst);
                else if (funct3 == F3_AMO)
                    exec_amo(cpu, &r_inst, funct5);
            }
            cpu->pc += 4;
            break;
            
        /* ==================== LOAD (0x03) ==================== */
        case OP_LOAD:
            {
                uint32_t addr = cpu->regs[i_inst.rs1] + i_inst.imm;
                if (addr < MEM_SIZE) {
                    switch (funct3) {
                        case 0x0:  /* LB */
                            cpu->regs[i_inst.rd] = sign_extend(
                                (cpu->memory[addr / 4] >> ((addr % 4) * 8)) & 0xFF, 8);
                            break;
                        case 0x1:  /* LH */
                            cpu->regs[i_inst.rd] = sign_extend(
                                (cpu->memory[addr / 4] >> ((addr % 4) * 8)) & 0xFFFF, 16);
                            break;
                        case 0x2:  /* LW */
                            cpu->regs[i_inst.rd] = cpu->memory[addr / 4];
                            break;
                    }
                }
            }
            cpu->pc += 4;
            break;
            
        /* ==================== STORE (0x23) ==================== */
        case OP_STORE:
            {
                int32_t imm = decode_store_imm(instr);
                uint32_t addr = cpu->regs[s_inst.rs1] + imm;
                if (addr < MEM_SIZE) {
                    uint32_t val = cpu->regs[s_inst.rs2];
                    uint32_t offset = (addr % 4) * 8;
                    switch (funct3) {
                        case 0x0:  /* SB */
                            cpu->memory[addr / 4] &= ~(0xFF << offset);
                            cpu->memory[addr / 4] |= (val & 0xFF) << offset;
                            break;
                        case 0x1:  /* SH */
                            cpu->memory[addr / 4] &= ~(0xFFFF << offset);
                            cpu->memory[addr / 4] |= (val & 0xFFFF) << offset;
                            break;
                        case 0x2:  /* SW */
                            cpu->memory[addr / 4] = val;
                            break;
                    }
                }
            }
            cpu->pc += 4;
            break;
            
        /* ==================== BRANCH (0x63) ==================== */
        case OP_BRANCH:
            {
                int32_t imm = decode_branch_imm(instr);
                int32_t rs1 = (int32_t)cpu->regs[s_inst.rs1];
                int32_t rs2 = (int32_t)cpu->regs[s_inst.rs2];
                int taken = 0;
                
                switch (funct3) {
                    case 0x0: taken = (rs1 == rs2); break;  /* BEQ */
                    case 0x1: taken = (rs1 != rs2); break;  /* BNE */
                    case 0x4: taken = (rs1 < rs2); break;   /* BLT */
                    case 0x5: taken = (rs1 >= rs2); break;  /* BGE */
                    case 0x6: taken = ((uint32_t)rs1 < (uint32_t)rs2); break;  /* BLTU */
                    case 0x7: taken = ((uint32_t)rs1 >= (uint32_t)rs2); break; /* BGEU */
                }
                
                cpu->pc += taken ? imm : 4;
            }
            break;
            
        /* ==================== JALR (0x67) ==================== */
        case OP_JALR:
            {
                uint32_t target = (cpu->regs[i_inst.rs1] + i_inst.imm) & ~1;
                cpu->regs[i_inst.rd] = cpu->pc + 4;
                cpu->pc = target;
            }
            break;
            
        /* ==================== JAL (0x6F) ==================== */
        case OP_JAL:
            {
                int32_t imm = decode_jal_imm(instr);
                cpu->regs[r_inst.rd] = cpu->pc + 4;
                cpu->pc += imm;
            }
            break;
            
        /* ==================== LUI (0x37) ==================== */
        case OP_LUI:
            cpu->regs[r_inst.rd] = instr & 0xFFFFF000;
            cpu->pc += 4;
            break;
            
        /* ==================== AUIPC (0x17) ==================== */
        case OP_AUIPC:
            cpu->regs[r_inst.rd] = cpu->pc + (instr & 0xFFFFF000);
            cpu->pc += 4;
            break;
            
        /* ==================== SYSTEM (0x73) ==================== */
        case OP_SYSTEM:
            if ((instr & 0xFE000FFF) == 0x00000073) {
                printf("\n[ECALL] 系统调用\n");
                cpu->halted = 1;
                return 0;
            }
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - LOAD_FP (0x07) ==================== */
        case OP_LOAD_FP:
            {
                uint32_t addr = cpu->regs[i_inst.rs1] + i_inst.imm;
                if (addr < MEM_SIZE)
                    cpu->fregs[i_inst.rd].u = cpu->memory[addr / 4];
            }
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - STORE_FP (0x27) ==================== */
        case OP_STORE_FP:
            {
                int32_t imm = decode_store_imm(instr);
                uint32_t addr = cpu->regs[s_inst.rs1] + imm;
                if (addr < MEM_SIZE)
                    cpu->memory[addr / 4] = cpu->fregs[s_inst.rs2].u;
            }
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - FMADD (0x43) ==================== */
        case OP_FMADD:
            exec_fmadd(cpu, &r_inst);
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - FMSUB (0x47) ==================== */
        case OP_FMSUB:
            exec_fmsub(cpu, &r_inst);
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - FNMSUB (0x4B) ==================== */
        case OP_FNMSUB:
            exec_fnmsub(cpu, &r_inst);
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - FNMADD (0x4F) ==================== */
        case OP_FNMADD:
            exec_fnmadd(cpu, &r_inst);
            cpu->pc += 4;
            break;
            
        /* ==================== F 扩展 - OP_FP (0x53) ==================== */
        case OP_OP_FP:
            {
                uint32_t fmt = (funct7 >> 2) & 0x7;
                uint32_t rm = r_inst.rs2;
                
                if (fmt == 0) {  /* 单精度 */
                    switch (funct7 & 0x60) {
                        case 0x00: exec_fadd(cpu, &r_inst); break;
                        case 0x04: exec_fsub(cpu, &r_inst); break;
                        case 0x08: exec_fmul(cpu, &r_inst); break;
                        case 0x0C: exec_fdiv(cpu, &r_inst); break;
                        case 0x14: exec_fsqrt(cpu, &r_inst); break;
                        case 0x10:  /* FSGNJ */
                            if (rm == 0) exec_fsgnj(cpu, &r_inst, 0, 0);
                            else if (rm == 1) exec_fsgnj(cpu, &r_inst, 1, 0);
                            else if (rm == 2) exec_fsgnj(cpu, &r_inst, 0, 1);
                            break;
                        case 0x18:  /* FMIN/FMAX */
                            if (rm == 0) exec_fmin(cpu, &r_inst);
                            else if (rm == 1) exec_fmax(cpu, &r_inst);
                            break;
                        case 0x20:  /* FCVT.W.S */
                            if (rm == 0) exec_fcvt_w_s(cpu, &r_inst, 0);
                            else if (rm == 1) exec_fcvt_w_s(cpu, &r_inst, 1);
                            break;
                        case 0x60:  /* FCVT.S.W */
                            if (rm == 0) exec_fcvt_s_w(cpu, &r_inst, 0);
                            else if (rm == 1) exec_fcvt_s_w(cpu, &r_inst, 1);
                            break;
                        case 0x50:  /* 浮点比较 */
                            if (rm == 1) exec_feq(cpu, &r_inst);
                            else if (rm == 2) exec_flt(cpu, &r_inst);
                            else if (rm == 3) exec_fle(cpu, &r_inst);
                            break;
                        case 0x58:  /* FCLASS */
                            if (rm == 0) exec_fclass(cpu, &r_inst);
                            break;
                    }
                }
            }
            cpu->pc += 4;
            break;
            
        /* ==================== 未知指令 ==================== */
        default:
            fprintf(stderr, "[EXECUTE] 未知指令：0x%08x (PC=0x%08x)\n", instr, cpu->pc);
            return -1;
    }
    
    /* x0 硬连线为 0 */
    cpu->regs[0] = 0;
    
    cpu->cycles++;
    return 1;
}
