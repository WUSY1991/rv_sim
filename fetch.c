/*
 * fetch.c - 取指模块
 * 负责从内存中获取指令
 */

#include "cpu.h"
#include <stdio.h>

/**
 * cpu_fetch - 从内存中取指
 * @cpu: CPU 状态结构体指针
 *
 * 返回：32 位指令，如果 PC 超出内存范围返回 0
 */
uint32_t cpu_fetch(CPU *cpu) {
    uint32_t fetch_addr = cpu->pc-0x1000000;
    /* 检查 PC 是否越界 */
    if (fetch_addr >= MEM_SIZE) {
        fprintf(stderr, "[FETCH] PC 越界：0x%08x\n", cpu->pc);
        return 0;
    }

    /* 检查 PC 是否对齐 (RISC-V 要求 4 字节对齐) */
    if (cpu->pc & 0x3) {
        fprintf(stderr, "[FETCH] PC 未对齐：0x%08x\n", cpu->pc);
        return 0;
    }

    /* 从内存中读取指令 (小端序) */
    uint32_t instr = cpu->memory[fetch_addr / 4];

#ifdef DEBUG_FETCH
    printf("[FETCH] PC=0x%08x, INSTR=0x%08x\n", cpu->pc, instr);
#endif

    return instr;
}
