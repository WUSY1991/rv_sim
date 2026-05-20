/*
 * test_simple.c - 简单测试
 */

#include <stdio.h>
#include <stdint.h>

static const uint32_t test_program[] = {
    0x00A00113,  /* addi  x2, x0, 10       ; x2 = 10 */
    0x00B00193,  /* addi  x3, x0, 11       ; x3 = 11 */
    0x00310233,  /* add   x4, x2, x3       ; x4 = 21 */
    0x00100073,  /* ebreak */
};

int main() {
    printf("=== 简单测试 ===\n\n");
    printf("预期: x2=10, x3=11, x4=21\n\n");

    FILE *f = fopen("test_simple.bin", "wb");
    fwrite(test_program, sizeof(uint32_t), 4, f);
    fclose(f);

    printf("已生成：test_simple.bin\n");
    printf("运行：./rv_sim test_simple.bin\n");
    return 0;
}