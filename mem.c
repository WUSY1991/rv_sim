/*
 * mem.c - 内存管理模块实现
 * 统一管理 IMEM/DMEM/UART 的读写访问
 * 取指令可以从 IMEM 或 DMEM 中获取
 */

#include "mem.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

/* ==================== 内存区域检测 ==================== */

/**
 * mem_is_imem - 检测地址是否在 IMEM 区域
 */
int mem_is_imem(uint32_t addr) {
    return addr >= IMEM_BASE_ADDR && addr < IMEM_BASE_ADDR + IMEM_SIZE;
}

/**
 * mem_is_dmem - 检测地址是否在 DMEM 区域
 */
int mem_is_dmem(uint32_t addr) {
    return addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE;
}

/**
 * mem_is_valid - 检测地址是否在有效内存区域 (IMEM 或 DMEM)
 */
int mem_is_valid(uint32_t addr) {
    return mem_is_imem(addr) || mem_is_dmem(addr);
}

/* ==================== 统一内存读取 ==================== */

/**
 * mem_read - 统一内存读取接口
 * @cpu: CPU 状态结构体指针
 * @addr: 物理地址
 * @size: 访问大小 (1=字节, 2=半字, 4=字)
 *
 * 返回：读取的数据 (未映射区域返回 0)
 */
uint32_t mem_read(CPU *cpu, uint32_t addr, int size) {

    uint32_t ret = 0;
    /* UART RX */
    if (addr == UART_RX_ADDR) {
        if (cpu->uart_rx_valid) {
            cpu->uart_rx_valid = 0;
            ret= cpu->uart_rx_char;
        }
        goto END;
    }

    /* IMEM 区域 */
    if (mem_is_imem(addr)) {
        uint32_t offset = addr - IMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        uint32_t word = cpu->imem[offset / 4];
        switch (size) {
            case 1: ret =  (word >> shift) & 0xFF;     break;
            case 2: ret =  (word >> shift) & 0xFFFF;   break;
            case 4: ret =  word;                       break;
            default: ret =0;                           break;
        }
        goto END;
    }

    /* DMEM 区域 */
    if (mem_is_dmem(addr)) {
        uint32_t offset = addr - DMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        uint32_t word = cpu->dmem[offset / 4];
        switch (size) {
            case 1: ret = (word >> shift) & 0xFF;       break;
            case 2: ret = (word >> shift) & 0xFFFF;     break;
            case 4: ret = word;                         break;
            default: ret =0;                            break;
        }
    }
END:
#ifdef MEM_TRACE
     printf("    [mr:0x%08x] 0x%08x-%d\n", addr, ret,size);
#endif
    for(int i = 0; i < 4; i++)
    {
        if(addr == cpu->breakpoint[i] && (cpu->brkctrl[i] & 0x03) == 0x02 )
        {
            printf("pc:0x%08x halt:0x%08x \n", cpu->pc, cpu->breakpoint[i]);
        }
    }
    return ret;  /* 未映射区域 */
}

/* ==================== 统一内存写入 ==================== */

/**
 * mem_write - 统一内存写入接口
 * @cpu: CPU 状态结构体指针
 * @addr: 物理地址
 * @value: 要写入的数据
 * @size: 访问大小 (1=字节, 2=半字, 4=字)
 */
void mem_write(CPU *cpu, uint32_t addr, uint32_t value, int size) {
#ifdef MEM_TRACE
     printf("    [mw:0x%08x] 0x%08x-%d\n", addr, value, size);
#endif
    for(int i = 0; i < 4; i++)
    {
        if(addr == cpu->breakpoint[i] && (cpu->brkctrl[i] & 0x03) == 0x01 )
        {
            printf("pc:0x%08x halt:0x%08x \n", cpu->pc, cpu->breakpoint[i]);
        }
    }

    /* UART TX */
    if (addr == UART_TX_ADDR) {
        cpu->uart_tx_char = value & 0xFF;
        cpu->uart_tx_ready = 1;
        putchar(cpu->uart_tx_char);
        fflush(stdout);
        return;
    }

    /* IMEM 区域 */
    if (mem_is_imem(addr)) {
        uint32_t offset = addr - IMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        switch (size) {
            case 1:
                cpu->imem[offset / 4] &= ~(0xFF << shift);
                cpu->imem[offset / 4] |= (value & 0xFF) << shift;
                break;
            case 2:
                cpu->imem[offset / 4] &= ~(0xFFFF << shift);
                cpu->imem[offset / 4] |= (value & 0xFFFF) << shift;
                break;
            case 4:
                cpu->imem[offset / 4] = value;
                break;
        }
        return;
    }

    /* DMEM 区域 */
    if (mem_is_dmem(addr)) {
        uint32_t offset = addr - DMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        switch (size) {
            case 1:
                cpu->dmem[offset / 4] &= ~(0xFF << shift);
                cpu->dmem[offset / 4] |= (value & 0xFF) << shift;
                break;
            case 2:
                cpu->dmem[offset / 4] &= ~(0xFFFF << shift);
                cpu->dmem[offset / 4] |= (value & 0xFFFF) << shift;
                break;
            case 4:
                cpu->dmem[offset / 4] = value;
                break;
        }
        return;
    }
}

/* ==================== 取指令 ==================== */

/**
 * mem_fetch_instr - 取指令 (支持从 IMEM 或 DMEM 取指令)
 * @cpu: CPU 状态结构体指针
 * @pc: 程序计数器 (物理地址)
 * @instr_len: 输出参数，返回指令长度 (2 或 4)
 *
 * 返回：指令 (压缩指令为 16 位，标准指令为 32 位)
 *       PC 越界时返回 0，instr_len = 0
 */
uint32_t mem_fetch_instr(CPU *cpu, uint32_t pc, int *instr_len) {
    /* PC 必须在 IMEM 或 DMEM 区域 */
    if (!mem_is_valid(pc)) {
        fprintf(stderr, "[FETCH] PC 越界：0x%08x\n", pc);
        *instr_len = 0;
        return 0;
    }

    /* PC 必须 2 字节对齐 (压缩指令要求) */
    if (pc & 0x1) {
        fprintf(stderr, "[FETCH] PC 未对齐：0x%08x\n", pc);
        *instr_len = 0;
        return 0;
    }

    uint32_t offset, word;
    uint16_t instr_low;

    /* 从 IMEM 或 DMEM 取指令 */
    if (mem_is_imem(pc)) {
        offset = pc - IMEM_BASE_ADDR;
        word = cpu->imem[offset / 4];
    } else {
        offset = pc - DMEM_BASE_ADDR;
        word = cpu->dmem[offset / 4];
    }

    /* 根据地址确定取低半字还是高半字 */
    if ((offset & 0x2) == 0) {
        instr_low = word & 0xFFFF;  /* 低半字 */
    } else {
        instr_low = (word >> 16) & 0xFFFF;  /* 高半字 */
    }

    /* 压缩指令检测 (最低两位 != 11) */
    if ((instr_low & 0x3) != 0x3) {
        *instr_len = 2;
        return instr_low;
    }

    /* 标准 32 位指令 */
    *instr_len = 4;
    if ((offset & 0x2) == 0) {
        /* 从字边界开始，直接返回整个字 */
        return word;
    } else {
        /* 从半字边界开始，需要组合两个半字 */
        uint32_t next_word;
        if (mem_is_imem(pc)) {
            next_word = cpu->imem[(offset + 2) / 4];
        } else {
            next_word = cpu->dmem[(offset + 2) / 4];
        }
        return instr_low | ((next_word & 0xFFFF) << 16);
    }
}

/* ==================== 程序加载 ==================== */

/**
 * mem_load_program - 加载程序到内存
 * @cpu: CPU 状态结构体指针
 * @program: 程序数据指针
 * @size: 程序大小 (字节)
 * @base_addr: 加载基地址
 *
 * 返回：0 表示成功，-1 表示失败
 */
int mem_load_program(CPU *cpu, const uint8_t *program, size_t size, uint32_t base_addr) {
    /* 检查目标区域 */
    if (mem_is_imem(base_addr)) {
        if (size > IMEM_SIZE) {
            fprintf(stderr, "[MEM] 程序太大：%lu 字节 (IMEM 最大 %d)\n",
                    (unsigned long)size, IMEM_SIZE);
            return -1;
        }
        uint32_t offset = base_addr - IMEM_BASE_ADDR;
        memcpy((uint8_t*)cpu->imem + offset, program, size);
        return 0;
    }

    if (mem_is_dmem(base_addr)) {
        if (size > DMEM_SIZE) {
            fprintf(stderr, "[MEM] 程序太大：%lu 字节 (DMEM 最大 %d)\n",
                    (unsigned long)size, DMEM_SIZE);
            return -1;
        }
        uint32_t offset = base_addr - DMEM_BASE_ADDR;
        memcpy((uint8_t*)cpu->dmem + offset, program, size);
        return 0;
    }

    fprintf(stderr, "[MEM] 加载地址无效：0x%08x\n", base_addr);
    return -1;
}