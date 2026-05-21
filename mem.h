/*
 * mem.h - 内存管理模块接口
 * 统一管理 IMEM/DMEM/UART 的读写访问
 */

#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

/* ========== 内存区域配置 ========== */

/* 指令内存 (IMEM) */
#define IMEM_BASE_ADDR  0x1000000   /* 指令内存基地址 */
#define IMEM_SIZE       (256 * 1024)  /* 指令内存大小 256KB */

/* 数据内存 (DMEM) */
#define DMEM_BASE_ADDR  0x10000     /* 数据内存基地址 */
#define DMEM_SIZE       (256 * 1024)  /* 数据内存大小 256KB */

/* PC 复位地址 */
#define PC_RESET_ADDR   0x1000080   /* PC 复位地址 */

/* ========== UART 虚拟串口 ========== */
#define UART_TX_ADDR    0x8200      /* 发送寄存器地址 */
#define UART_RX_ADDR    0x8210      /* 接收寄存器地址 */

/* ========== 前向声明 ========== */
struct CPU;

/* ========== 函数声明 ========== */

/* 内存区域检测 */
int mem_is_imem(uint32_t addr);     /* 地址在 IMEM 区域 */
int mem_is_dmem(uint32_t addr);     /* 地址在 DMEM 区域 */
int mem_is_valid(uint32_t addr);    /* 地址在有效内存区域 */

/* 统一内存读取 */
uint32_t mem_read(struct CPU *cpu, uint32_t addr, int size);

/* 统一内存写入 */
void mem_write(struct CPU *cpu, uint32_t addr, uint32_t value, int size);

/* 取指令 (支持从 IMEM 或 DMEM 取指令) */
uint32_t mem_fetch_instr(struct CPU *cpu, uint32_t pc, int *instr_len);

/* 程序加载 */
int mem_load_program(struct CPU *cpu, const uint8_t *program, size_t size, uint32_t base_addr);

#endif /* MEM_H */