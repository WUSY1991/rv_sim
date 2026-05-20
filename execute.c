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

/* ==================== UART 虚拟串口 ==================== */

/**
 * uart_write_tx - UART 发送字符
 * @cpu: CPU 状态结构体指针
 * @value: 要发送的字符
 */
static void uart_write_tx(CPU *cpu, uint8_t value) {
    cpu->uart_tx_char = value;
    cpu->uart_tx_ready = 1;
    /* 发送字符到控制台 */
    putchar(value);
    fflush(stdout);
}

/**
 * uart_read_rx - UART 接收字符
 * @cpu: CPU 状态结构体指针
 *
 * 返回：接收到的字符（如果没有数据返回 0）
 */
static uint8_t uart_read_rx(CPU *cpu) {
    if (cpu->uart_rx_valid) {
        cpu->uart_rx_valid = 0;
        return cpu->uart_rx_char;
    }
    return 0;
}

/* ==================== 内存访问辅助函数 ==================== */

/**
 * dmem_read - 从数据内存读取
 * @cpu: CPU 状态结构体指针
 * @addr: 物理地址
 * @size: 访问大小 (1=字节, 2=半字, 4=字)
 *
 * 返回：读取的数据
 */
static uint32_t dmem_read(CPU *cpu, uint32_t addr, int size) {
    uint32_t dmem_addr = addr - DMEM_BASE_ADDR;
    if (dmem_addr >= DMEM_SIZE) return 0;

    uint32_t word = cpu->dmem[dmem_addr / 4];
    uint32_t offset = (dmem_addr % 4) * 8;

    switch (size) {
        case 1: return (word >> offset) & 0xFF;
        case 2: return (word >> offset) & 0xFFFF;
        case 4: return word;
        default: return 0;
    }
}

/**
 * dmem_write - 向数据内存写入
 * @cpu: CPU 状态结构体指针
 * @addr: 物理地址
 * @value: 要写入的数据
 * @size: 访问大小 (1=字节, 2=半字, 4=字)
 */
static void dmem_write(CPU *cpu, uint32_t addr, uint32_t value, int size) {
    uint32_t dmem_addr = addr - DMEM_BASE_ADDR;
    if (dmem_addr >= DMEM_SIZE) return;

    uint32_t offset = (dmem_addr % 4) * 8;

    switch (size) {
        case 1:
            cpu->dmem[dmem_addr / 4] &= ~(0xFF << offset);
            cpu->dmem[dmem_addr / 4] |= (value & 0xFF) << offset;
            break;
        case 2:
            cpu->dmem[dmem_addr / 4] &= ~(0xFFFF << offset);
            cpu->dmem[dmem_addr / 4] |= (value & 0xFFFF) << offset;
            break;
        case 4:
            cpu->dmem[dmem_addr / 4] = value;
            break;
    }
}

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

    /* 数据内存 (DMEM) */
    if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
        cpu->regs[r->rd] = dmem_read(cpu, addr, 4);
        cpu->has_reservation = 1;
        cpu->reservation_addr = addr;
    }
}

static void exec_sc_w(CPU *cpu, RType *r) {
    uint32_t addr = cpu->regs[r->rs1];

    /* 数据内存 (DMEM) */
    if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
        if (cpu->has_reservation && cpu->reservation_addr == addr) {
            dmem_write(cpu, addr, cpu->regs[r->rs2], 4);
            cpu->regs[r->rd] = 0;
        } else {
            cpu->regs[r->rd] = 1;
        }
        cpu->has_reservation = 0;
    } else {
        cpu->regs[r->rd] = 1;
    }
}

static void exec_amo(CPU *cpu, RType *r, uint32_t funct5) {
    uint32_t addr = cpu->regs[r->rs1];

    /* 数据内存 (DMEM) */
    if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
        uint32_t dmem_addr = addr - DMEM_BASE_ADDR;
        uint32_t *mem_loc = &cpu->dmem[dmem_addr / 4];
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

static void exec_fmv_x_w(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->fregs[r->rs1].u;  /* 位拷贝 */
}

static void exec_fmv_w_x(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].u = cpu->regs[r->rs1];  /* 位拷贝 */
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

/* ==================== CSR 寄存器操作 ==================== */

static uint32_t csr_read(CPU *cpu, uint32_t csr_addr) {
    switch (csr_addr) {
        case CSR_FFLAGS:
            return cpu->fcsr & 0x1F;  /* FFLAGS = FCSR[4:0] */
        case CSR_FRM:
            return (cpu->fcsr >> 5) & 0x7;  /* FRM = FCSR[7:5] */
        case CSR_FCSR:
            return cpu->fcsr;  /* FCSR 完整值 */
        default:
            return 0;  /* 未实现的 CSR 返回 0 */
    }
}

static void csr_write(CPU *cpu, uint32_t csr_addr, uint32_t value) {
    switch (csr_addr) {
        case CSR_FFLAGS:
            cpu->fcsr = (cpu->fcsr & ~0x1F) | (value & 0x1F);
            break;
        case CSR_FRM:
            cpu->fcsr = (cpu->fcsr & ~0xE0) | ((value & 0x7) << 5);
            break;
        case CSR_FCSR:
            cpu->fcsr = value & 0xFF;  /* FCSR 只有低 8 位有效 */
            break;
        /* 其他 CSR 不实现，写入忽略 */
    }
}

static void exec_csrrw(CPU *cpu, uint32_t rd, uint32_t rs1, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    if (rs1 != 0) {  /* rs1=0 时只读不写 */
        csr_write(cpu, csr_addr, cpu->regs[rs1]);
    }
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrs(CPU *cpu, uint32_t rd, uint32_t rs1, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    if (rs1 != 0) {  /* rs1=0 时只读不置位 */
        csr_write(cpu, csr_addr, old_val | cpu->regs[rs1]);
    }
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrc(CPU *cpu, uint32_t rd, uint32_t rs1, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    if (rs1 != 0) {  /* rs1=0 时只读不清位 */
        csr_write(cpu, csr_addr, old_val & ~cpu->regs[rs1]);
    }
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

/* CSR 立即数版本 (rs1 作为 uimm[4:0]) */
static void exec_csrrwi(CPU *cpu, uint32_t rd, uint32_t uimm, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    csr_write(cpu, csr_addr, uimm);  /* 立即数直接写入 */
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrsi(CPU *cpu, uint32_t rd, uint32_t uimm, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    csr_write(cpu, csr_addr, old_val | uimm);
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrci(CPU *cpu, uint32_t rd, uint32_t uimm, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    csr_write(cpu, csr_addr, old_val & ~uimm);
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
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
 * @instr: 指令 (压缩指令为 16 位，标准指令为 32 位)
 * @instr_len: 指令长度 (2 或 4)
 *
 * 返回：1 表示继续执行，0 表示停止 (ECALL/EBREAK)，-1 表示错误
 */
int cpu_execute(CPU *cpu, uint32_t instr, int instr_len) {
    /* 压缩指令展开为 32 位等效指令 */
    uint32_t expanded_instr;
    int actual_len;

    if (instr_len == 2) {
        expanded_instr = expand_compressed((uint16_t)instr);
        if (expanded_instr == 0) {
            fprintf(stderr, "[EXECUTE] 非法压缩指令：0x%04x\n", (uint16_t)instr);
            return -1;
        }
        actual_len = 2;
#ifdef DEBUG_COMPRESSED
        printf("[COMPRESSED] 0x%04x -> 0x%08x\n", (uint16_t)instr, expanded_instr);
#endif
    } else {
        expanded_instr = instr;
        actual_len = 4;
    }

    /* 统一执行 32 位指令 */
    RType r_inst;
    IType i_inst;
    SType s_inst;

    cpu_decode(cpu, expanded_instr, &r_inst, &i_inst, &s_inst, NULL);

    uint32_t opcode = r_inst.opcode;
    uint32_t funct3 = r_inst.funct3;
    uint32_t funct7 = r_inst.funct7;
    
    switch (opcode) {
        /* ==================== OP (0x33) ==================== */
        case OP_OP:
            /* M 扩展 (funct7 = 0x01) */
            if (funct7 == 0x01) {
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
            } else {
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
            }
            cpu->pc += actual_len;
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
            cpu->pc += actual_len;
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
            cpu->pc += actual_len;
            break;
            
        /* ==================== LOAD (0x03) ==================== */
        case OP_LOAD:
            {
                uint32_t addr = cpu->regs[i_inst.rs1] + i_inst.imm;

                /* UART 接收 */
                if (addr == UART_RX_ADDR) {
                    cpu->regs[i_inst.rd] = uart_read_rx(cpu);
                }
                /* 数据内存 (DMEM) */
                else if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
                    switch (funct3) {
                        case 0x0:  /* LB */
                            cpu->regs[i_inst.rd] = sign_extend(dmem_read(cpu, addr, 1), 8);
                            break;
                        case 0x1:  /* LH */
                            cpu->regs[i_inst.rd] = sign_extend(dmem_read(cpu, addr, 2), 16);
                            break;
                        case 0x2:  /* LW */
                            cpu->regs[i_inst.rd] = dmem_read(cpu, addr, 4);
                            break;
                        case 0x4:  /* LBU */
                            cpu->regs[i_inst.rd] = dmem_read(cpu, addr, 1);
                            break;
                        case 0x5:  /* LHU */
                            cpu->regs[i_inst.rd] = dmem_read(cpu, addr, 2);
                            break;
                    }
                }
                /* 指令内存 (IMEM) - 用于调试/读取指令 */
                else if (addr >= IMEM_BASE_ADDR && addr < IMEM_BASE_ADDR + IMEM_SIZE) {
                    uint32_t imem_addr = addr - IMEM_BASE_ADDR;
                    switch (funct3) {
                        case 0x0:  /* LB */
                            cpu->regs[i_inst.rd] = sign_extend(
                                (cpu->imem[imem_addr / 4] >> ((imem_addr % 4) * 8)) & 0xFF, 8);
                            break;
                        case 0x1:  /* LH */
                            cpu->regs[i_inst.rd] = sign_extend(
                                (cpu->imem[imem_addr / 4] >> ((imem_addr % 4) * 8)) & 0xFFFF, 16);
                            break;
                        case 0x2:  /* LW */
                            cpu->regs[i_inst.rd] = cpu->imem[imem_addr / 4];
                            break;
                        case 0x4:  /* LBU */
                            cpu->regs[i_inst.rd] = (cpu->imem[imem_addr / 4] >> ((imem_addr % 4) * 8)) & 0xFF;
                            break;
                        case 0x5:  /* LHU */
                            cpu->regs[i_inst.rd] = (cpu->imem[imem_addr / 4] >> ((imem_addr % 4) * 8)) & 0xFFFF;
                            break;
                    }
                }
            }
            cpu->pc += actual_len;
            break;
            
        /* ==================== STORE (0x23) ==================== */
        case OP_STORE:
            {
                int32_t imm = decode_store_imm(expanded_instr);
                uint32_t addr = cpu->regs[s_inst.rs1] + imm;
                uint32_t val = cpu->regs[s_inst.rs2];

                /* UART 发送 */
                if (addr == UART_TX_ADDR) {
                    uart_write_tx(cpu, val & 0xFF);
                }
                /* 数据内存 (DMEM) */
                else if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
                    switch (funct3) {
                        case 0x0:  /* SB */
                            dmem_write(cpu, addr, val, 1);
                            break;
                        case 0x1:  /* SH */
                            dmem_write(cpu, addr, val, 2);
                            break;
                        case 0x2:  /* SW */
                            dmem_write(cpu, addr, val, 4);
                            break;
                    }
                }
                /* 指令内存 (IMEM) - 用于调试/修改指令 */
                else if (addr >= IMEM_BASE_ADDR && addr < IMEM_BASE_ADDR + IMEM_SIZE) {
                    uint32_t imem_addr = addr - IMEM_BASE_ADDR;
                    uint32_t offset = (imem_addr % 4) * 8;
                    switch (funct3) {
                        case 0x0:  /* SB */
                            cpu->imem[imem_addr / 4] &= ~(0xFF << offset);
                            cpu->imem[imem_addr / 4] |= (val & 0xFF) << offset;
                            break;
                        case 0x1:  /* SH */
                            cpu->imem[imem_addr / 4] &= ~(0xFFFF << offset);
                            cpu->imem[imem_addr / 4] |= (val & 0xFFFF) << offset;
                            break;
                        case 0x2:  /* SW */
                            cpu->imem[imem_addr / 4] = val;
                            break;
                    }
                }
            }
            cpu->pc += actual_len;
            break;
            
        /* ==================== BRANCH (0x63) ==================== */
        case OP_BRANCH:
            {
                int32_t imm = decode_branch_imm(expanded_instr);
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
                cpu->regs[i_inst.rd] = cpu->pc + actual_len;
                cpu->pc = target;
            }
            break;

        /* ==================== JAL (0x6F) ==================== */
        case OP_JAL:
            {
                int32_t imm = decode_jal_imm(expanded_instr);
                cpu->regs[r_inst.rd] = cpu->pc + actual_len;
                cpu->pc += imm;
            }
            break;
            
        /* ==================== LUI (0x37) ==================== */
        case OP_LUI:
            cpu->regs[r_inst.rd] = expanded_instr & 0xFFFFF000;
            cpu->pc += actual_len;
            break;

        /* ==================== AUIPC (0x17) ==================== */
        case OP_AUIPC:
            cpu->regs[r_inst.rd] = cpu->pc + (expanded_instr & 0xFFFFF000);
            cpu->pc += actual_len;
            break;
            
        /* ==================== SYSTEM (0x73) ==================== */
        case OP_SYSTEM:
            {
                uint32_t csr_addr = (instr >> 20) & 0xFFF;  /* CSR 地址 */
                uint32_t rd = r_inst.rd;
                uint32_t rs1 = r_inst.rs1;
                uint32_t uimm = rs1;  /* 立即数版本使用 rs1 作为 uimm[4:0] */

                if (expanded_instr == 0x00000073) {  /* ECALL */
                    printf("\n[ECALL] 系统调用\n");
                    cpu->halted = 1;
                    return 0;
                } else if (expanded_instr == 0x00100073) {  /* EBREAK */
                    printf("\n[EBREAK] 断点\n");
                    cpu->halted = 1;
                    return 0;
                } else if (funct3 == F3_CSRRW) {
                    exec_csrrw(cpu, rd, rs1, csr_addr);
                } else if (funct3 == F3_CSRRS) {
                    exec_csrrs(cpu, rd, rs1, csr_addr);
                } else if (funct3 == F3_CSRRC) {
                    exec_csrrc(cpu, rd, rs1, csr_addr);
                } else if (funct3 == F3_CSRRWI) {
                    exec_csrrwi(cpu, rd, uimm, csr_addr);
                } else if (funct3 == F3_CSRRSI) {
                    exec_csrrsi(cpu, rd, uimm, csr_addr);
                } else if (funct3 == F3_CSRRCI) {
                    exec_csrrci(cpu, rd, uimm, csr_addr);
                } else {
                    fprintf(stderr, "[EXECUTE] 未知 SYSTEM 指令：0x%08x (PC=0x%08x)\n", expanded_instr, cpu->pc);
                }
            }
            cpu->pc += actual_len;
            break;
            
        /* ==================== F 扩展 - LOAD_FP (0x07) ==================== */
        case OP_LOAD_FP:
            {
                uint32_t addr = cpu->regs[i_inst.rs1] + i_inst.imm;

                /* 数据内存 (DMEM) */
                if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
                    cpu->fregs[i_inst.rd].u = dmem_read(cpu, addr, 4);
                }
                /* 指令内存 (IMEM) - 用于调试 */
                else if (addr >= IMEM_BASE_ADDR && addr < IMEM_BASE_ADDR + IMEM_SIZE) {
                    uint32_t imem_addr = addr - IMEM_BASE_ADDR;
                    cpu->fregs[i_inst.rd].u = cpu->imem[imem_addr / 4];
                }
            }
            cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - STORE_FP (0x27) ==================== */
        case OP_STORE_FP:
            {
                int32_t imm = decode_store_imm(expanded_instr);
                uint32_t addr = cpu->regs[s_inst.rs1] + imm;
                uint32_t val = cpu->fregs[s_inst.rs2].u;

                /* UART 发送 (浮点寄存器写入也支持 UART) */
                if (addr == UART_TX_ADDR) {
                    uart_write_tx(cpu, val & 0xFF);
                }
                /* 数据内存 (DMEM) */
                else if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
                    dmem_write(cpu, addr, val, 4);
                }
                /* 指令内存 (IMEM) - 用于调试 */
                else if (addr >= IMEM_BASE_ADDR && addr < IMEM_BASE_ADDR + IMEM_SIZE) {
                    uint32_t imem_addr = addr - IMEM_BASE_ADDR;
                    cpu->imem[imem_addr / 4] = val;
                }
            }
            cpu->pc += actual_len;
            break;
            
        /* ==================== F 扩展 - FMADD (0x43) ==================== */
        case OP_FMADD:
            exec_fmadd(cpu, &r_inst);
            cpu->pc += actual_len;
            break;
            
        /* ==================== F 扩展 - FMSUB (0x47) ==================== */
        case OP_FMSUB:
            exec_fmsub(cpu, &r_inst);
            cpu->pc += actual_len;
            break;
            
        /* ==================== F 扩展 - FNMSUB (0x4B) ==================== */
        case OP_FNMSUB:
            exec_fnmsub(cpu, &r_inst);
            cpu->pc += actual_len;
            break;
            
        /* ==================== F 扩展 - FNMADD (0x4F) ==================== */
        case OP_FNMADD:
            exec_fnmadd(cpu, &r_inst);
            cpu->pc += actual_len;
            break;
            
        /* ==================== F 扩展 - OP_FP (0x53) ==================== */
        case OP_OP_FP:
            {
                uint32_t funct5 = funct7 >> 2;
                uint32_t fmt = funct7 & 0x03;
                uint32_t rm = r_inst.rs2;

                if (fmt == 0) {  /* 单精度 (fmt=0) */
                    switch (funct5) {
                        case 0x00:  /* FADD.S */
                            exec_fadd(cpu, &r_inst);
                            break;
                        case 0x01:  /* FSUB.S */
                            exec_fsub(cpu, &r_inst);
                            break;
                        case 0x02:  /* FMUL.S */
                            exec_fmul(cpu, &r_inst);
                            break;
                        case 0x03:  /* FDIV.S */
                            exec_fdiv(cpu, &r_inst);
                            break;
                        case 0x04:  /* FSGNJ.S (rm=0), FSGNJN.S (rm=1), FSGNJX.S (rm=2) */
                            if (rm == 0) exec_fsgnj(cpu, &r_inst, 0, 0);
                            else if (rm == 1) exec_fsgnj(cpu, &r_inst, 1, 0);
                            else if (rm == 2) exec_fsgnj(cpu, &r_inst, 0, 1);
                            break;
                        case 0x05:  /* FMIN.S (rm=0), FMAX.S (rm=1) */
                            if (rm == 0) exec_fmin(cpu, &r_inst);
                            else if (rm == 1) exec_fmax(cpu, &r_inst);
                            break;
                        case 0x06:  /* FSQRT.S */
                            exec_fsqrt(cpu, &r_inst);
                            break;
                        case 0x07:  /* FLE.S (rm=0), FLT.S (rm=1), FEQ.S (rm=2) */
                            if (rm == 0) exec_fle(cpu, &r_inst);
                            else if (rm == 1) exec_flt(cpu, &r_inst);
                            else if (rm == 2) exec_feq(cpu, &r_inst);
                            break;
                        case 0x08:  /* FCVT.W.S (rs2=signed/unsigned) */
                            if (rm == 0) exec_fcvt_w_s(cpu, &r_inst, 0);
                            else if (rm == 1) exec_fcvt_w_s(cpu, &r_inst, 1);
                            break;
                        case 0x09:  /* FCVT.WU.S */
                            exec_fcvt_w_s(cpu, &r_inst, 1);
                            break;
                        case 0x0A:  /* FCLASS */
                            exec_fclass(cpu, &r_inst);
                            break;
                        case 0x0E:  /* FCVT.S.W (rs2=signed/unsigned) */
                            if (rm == 0) exec_fcvt_s_w(cpu, &r_inst, 0);
                            else if (rm == 1) exec_fcvt_s_w(cpu, &r_inst, 1);
                            break;
                        case 0x0F:  /* FCVT.S.WU */
                            exec_fcvt_s_w(cpu, &r_inst, 1);
                            break;
                        case 0x1C:  /* FMV.X.W */
                            exec_fmv_x_w(cpu, &r_inst);
                            break;
                        case 0x1E:  /* FMV.W.X */
                            exec_fmv_w_x(cpu, &r_inst);
                            break;
                        default:
                            fprintf(stderr, "[EXECUTE] 未知 F 指令：funct5=0x%02x\n", funct5);
                            break;
                    }
                }
            }
            cpu->pc += actual_len;
            break;
            
        /* ==================== 未知指令 ==================== */
        default:
            fprintf(stderr, "[EXECUTE] 未知指令：0x%08x (PC=0x%08x)\n", expanded_instr, cpu->pc);
            return -1;
    }
    
    /* x0 硬连线为 0 */
    cpu->regs[0] = 0;
    
    cpu->cycles++;
    return 1;
}
