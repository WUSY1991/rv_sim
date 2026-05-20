/*
 * fetch.c - 取指模块
 * 负责从内存中获取指令，支持压缩指令(C扩展)
 */

#include "cpu.h"
#include <stdio.h>

/**
 * is_compressed - 检测是否为压缩指令
 * @instr_low: 指令低 16 位
 *
 * 返回：1 表示压缩指令，0 表示标准 32 位指令
 *
 * 压缩指令最低 2 位 != 11
 */
int is_compressed(uint16_t instr_low) {
    return (instr_low & 0x3) != 0x3;
}

/**
 * cpu_fetch - 从内存中取指
 * @cpu: CPU 状态结构体指针
 * @instr_len: 输出参数，返回指令长度（2 或 4）
 *
 * 返回：32 位指令（压缩指令会扩展为等效的 32 位格式），如果 PC 超出内存范围返回 0
 */
uint32_t cpu_fetch(CPU *cpu, int *instr_len) {
    uint32_t fetch_addr = cpu->pc - MEM_BASE_ADDR;

    /* 检查 PC 是否越界 */
    if (fetch_addr >= MEM_SIZE) {
        fprintf(stderr, "[FETCH] PC 越界：0x%08x\n", cpu->pc);
        return 0;
    }

    /* 检查 PC 是否对齐 (压缩指令要求 2 字节对齐) */
    if (cpu->pc & 0x1) {
        fprintf(stderr, "[FETCH] PC 未对齐：0x%08x\n", cpu->pc);
        return 0;
    }

    /* 从内存中读取指令低 16 位 */
    /* 内存按 32 位字存储，需要处理半字访问 */
    uint32_t word = cpu->memory[fetch_addr / 4];
    uint16_t instr_low;

    /* 根据地址确定取低半字还是高半字 */
    if ((fetch_addr & 0x2) == 0) {
        instr_low = word & 0xFFFF;  /* 低半字 */
    } else {
        instr_low = (word >> 16) & 0xFFFF;  /* 高半字 */
    }

#ifdef DEBUG_FETCH
    printf("[FETCH] PC=0x%08x, INSTR_LOW=0x%04x\n", cpu->pc, instr_low);
#endif

    /* 检测是否为压缩指令 */
    if (is_compressed(instr_low)) {
        *instr_len = 2;
        /* 压缩指令：返回 16 位指令（高位清零） */
        return instr_low;
    } else {
        *instr_len = 4;
        /* 标准 32 位指令 */
        /* 如果 instr_low 的最低两位是 11，则这是一条 32 位指令的开始 */
        uint32_t instr_high = 0;
        if ((fetch_addr & 0x2) == 0) {
            /* 从字边界开始，直接返回整个字 */
            instr_high = (word >> 16) & 0xFFFF;
            return instr_low | (instr_high << 16);
        } else {
            /* 从半字边界开始，需要组合两个半字 */
            if (fetch_addr + 2 < MEM_SIZE) {
                uint32_t next_word = cpu->memory[(fetch_addr + 2) / 4];
                instr_high = next_word & 0xFFFF;
                return instr_low | (instr_high << 16);
            }
            return instr_low;  /* 边界情况 */
        }
    }
}