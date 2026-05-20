/*
 * cpu.c - CPU 核心初始化和运行控制
 */

#include "cpu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 外部函数声明 */
extern uint32_t cpu_fetch(CPU *cpu);
extern int cpu_execute(CPU *cpu, uint32_t instr);
extern void cpu_dump(CPU *cpu);

/**
 * cpu_init - 初始化 CPU 状态
 * @cpu: CPU 状态结构体指针
 * 
 * 将所有寄存器、内存和状态清零
 */
void cpu_init(CPU *cpu) {
    /* 清零通用寄存器 */
    memset(cpu->regs, 0, sizeof(cpu->regs));
    
    /* 清零浮点寄存器 */
    memset(cpu->fregs, 0, sizeof(cpu->fregs));
    
    /* 清零内存 */
    memset(cpu->memory, 0, sizeof(cpu->memory));
    
    /* 初始化 PC */
    cpu->pc = 0x1000080;
    
    /* 清零 FCSR */
    cpu->fcsr = 0;
    
    /* 清除原子操作保留状态 */
    cpu->has_reservation = 0;
    cpu->reservation_addr = 0;
    
    /* 运行状态 */
    cpu->halted = 0;
    cpu->cycles = 0;
    
    /* x0 硬连线为 0 */
    cpu->regs[0] = 0;
}

/**
 * cpu_load_program - 加载程序到内存
 * @cpu: CPU 状态结构体指针
 * @program: 程序数据指针
 * @size: 程序大小 (字节)
 * 
 * 返回：0 表示成功，-1 表示失败
 */
int cpu_load_program(CPU *cpu, const uint32_t *program, size_t size) {
    if (size > MEM_SIZE) {
        fprintf(stderr, "[CPU] 程序太大：%zu 字节 (最大 %d)\n", size, MEM_SIZE);
        return -1;
    }
    
    /* 按字复制 */
    size_t num_words = size / sizeof(uint32_t);
    memcpy(cpu->memory, program, num_words * sizeof(uint32_t));
    
    return 0;
}

/**
 * cpu_load_binary - 从文件加载二进制程序
 * @cpu: CPU 状态结构体指针
 * @filename: 文件名
 * 
 * 返回：0 表示成功，-1 表示失败
 */
int cpu_load_binary(CPU *cpu, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[CPU] 无法打开文件：%s\n", filename);
        return -1;
    }
    
    /* 获取文件大小 */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size > MEM_SIZE) {
        fprintf(stderr, "[CPU] 文件太大：%ld 字节\n", size);
        fclose(f);
        return -1;
    }
    
    /* 读取文件 */
    uint32_t *prog = malloc(size);
    if (!prog) {
        fprintf(stderr, "[CPU] 内存分配失败\n");
        fclose(f);
        return -1;
    }
    
    size_t read = fread(prog, 1, size, f);
    fclose(f);
    
    if (read != (size_t)size) {
        fprintf(stderr, "[CPU] 读取文件失败\n");
        free(prog);
        return -1;
    }
    
    /* 加载到内存 */
    cpu_load_program(cpu, prog, size);
    free(prog);
    
    return 0;
}

/**
 * cpu_run - 运行 CPU
 * @cpu: CPU 状态结构体指针
 * @max_cycles: 最大执行周期数
 * 
 * 取指 - 译码 - 执行 - 写回循环
 */
void cpu_run(CPU *cpu, int max_cycles) {
    int cycles = 0;
    
    printf("========================================\n");
    printf("  RISC-V RV32IMACF 模拟器\n");
    printf("========================================\n\n");
    printf("开始执行 (PC=0x%08x)...\n\n", cpu->pc);
    
    while (cycles < max_cycles && !cpu->halted) {
        /* 取指 */
        uint32_t instr = cpu_fetch(cpu);
        if (instr == 0) {
            printf("[CPU] 遇到零指令，停止执行\n");
            break;
        }
        
        /* 打印当前指令 (调试用) */
#ifdef DEBUG_TRACE
        printf("[0x%08x] 0x%08x\n", cpu->pc, instr);
#endif
        
        /* 执行 (包含译码和写回) */
        int result = cpu_execute(cpu, instr);
        if (result <= 0) {
            break;
        }
        
        cycles++;
    }
    
    printf("\n========================================\n");
    printf("  完成 (%d 周期)\n", cycles);
    printf("========================================\n");
    
    cpu->cycles = cycles;
    
    /* 打印寄存器状态 */
    cpu_dump(cpu);
}

/**
 * cpu_dump - 打印 CPU 状态
 * @cpu: CPU 状态结构体指针
 */
void cpu_dump(CPU *cpu) {
    printf("\n=== 通用寄存器 ===\n");
    int has_nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (cpu->regs[i] != 0) {
            printf("  x%-2d = 0x%08x (%10d)\n", i, cpu->regs[i], (int32_t)cpu->regs[i]);
            has_nonzero = 1;
        }
    }
    if (!has_nonzero) {
        printf("  (全部为零)\n");
    }
    
    printf("\n=== 浮点寄存器 ===\n");
    has_nonzero = 0;
    for (int i = 0; i < 32; i++) {
        if (cpu->fregs[i].u != 0) {
            printf("  f%-2d = 0x%08x (%.6f)\n", i, cpu->fregs[i].u, cpu->fregs[i].f);
            has_nonzero = 1;
        }
    }
    if (!has_nonzero) {
        printf("  (全部为零)\n");
    }
    
    printf("\n=== 状态 ===\n");
    printf("  PC    = 0x%08x\n", cpu->pc);
    printf("  FCSR  = 0x%08x\n", cpu->fcsr);
    printf("  周期数 = %d\n", cpu->cycles);
    if (cpu->has_reservation) {
        printf("  保留地址 = 0x%08x\n", cpu->reservation_addr);
    }
}

/**
 * cpu_step - 单步执行
 * @cpu: CPU 状态结构体指针
 * 
 * 返回：1 表示继续，0 表示停止
 */
int cpu_step(CPU *cpu) {
    if (cpu->halted) {
        return 0;
    }
    
    uint32_t instr = cpu_fetch(cpu);
    if (instr == 0) {
        return 0;
    }
    
    printf("[0x%08x] 0x%08x\n", cpu->pc, instr);
    
    int result = cpu_execute(cpu, instr);
    return result > 0;
}
