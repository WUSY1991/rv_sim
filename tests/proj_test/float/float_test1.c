#include "float_test1.h"
#include "../types.h"
#include "../riscv_encoding.h"

void static inline set_csr_bit(const int csr, int bit)
{
    __asm("csrs %0,%1" :: "i"(csr), "r"(bit));
}

// typedef unsigned char uint8_t;
// typedef          char  int8_t;

// static float sqrtx(float b)
// {
//     float x = 1;
//     int step = 0;
//     while ((x * x - b < -0.000001 || x * x - b > 0.000001) && step < 50)
//     {
//         x = (b / x + x) / 2.0;
//         step++;
//     }
//     return x;
// }

// #define USE_PSRAM_ADDR
void test1(void)
{
    // static float t;
    // t = sqrtx(18);
    // yc_log("test1\r\n");
}



void fcvt_test(void)
{
    __asm("sw ra,0(sp)");
    __asm("li a4,0x4135");
    __asm("fcvt.s.w fa0,a4");
#ifdef USE_PSRAM_ADDR
    __asm("li s6,0x108a0000");
#else
    __asm("li s6,0x10015000");
#endif
    __asm("la a4,test1");
    __asm("sw a4,0(s6)");
    __asm("fmv.s	fs0,fa0");
    __asm("lw	    a5,0(s6)");
    __asm("fcvt.w.s	s1,fa0,rtz");
    __asm("jalr	a5");

    /*check a1*/
    __asm("li a4,0x4135");
    __asm("bne s1,a4,.");

    /*check fs0*/
    __asm("fcvt.w.s	a1,fs0,rtz");
    __asm("li a4,0x4135");
    __asm("bne a1,a4,.");

    // __asm("fcvt_ok:");
    __asm("lw ra,0(sp)");
}

void fcvt1_test(void)
{
    __asm("sw ra,0(sp)");
    __asm("li a1,0x4135");
    __asm("fcvt.s.w fa0,a1");
#ifdef USE_PSRAM_ADDR
    __asm("li s6,0x108a0000");
#else
    __asm("li s6,0x10015000");
#endif
    __asm("la a4,test1");
    __asm("sw a4,0(s6)");
    __asm("fmv.s	fs0,fa0");
    __asm("lw	    a5,0(s6)");
    __asm("fcvt.s.w	fa0,a1");
    __asm("jalr	a5");

    /*check fa0*/
    __asm("fcvt.w.s	a1,fa0,rtz");
    __asm("li a4,0x4135");
    __asm("beq a1,a4,fcvt1_ok");
    __asm("j .");

    __asm("fcvt1_ok:");
    __asm("lw ra,0(sp)");
}

void fcvt2_test(void)
{
    __asm("sw ra,0(sp)");
    __asm("li a4,0x4135");
    __asm("fcvt.s.w fs0,a4");
#ifdef USE_PSRAM_ADDR
    __asm("li s0,0x108a0000");
#else
    __asm("li s0,0x10015000");
#endif
    __asm("la a5,test1");

    __asm("lw a5,0(s0)");
    __asm("fcvt.w.s	a1,fs0,rtz"); // fail result : [a1] will set 0
    __asm("lui a0,0x1018");
    __asm("addi a0,a0,-72");

    __asm("beq a1,a4,f_ok");
    __asm("j .");
    __asm("f_ok:");
    __asm("lw ra,0(sp)");
}

void fcvt3_test(void)
{
	int i = 0x4135, j;
	volatile  uint8_t test[10] = {0};
	set_csr_bit(CSR_MSTATUS, 0x6000);
	__asm("csrw fcsr,x0");
	volatile float f1=3.33, f2=2.00;
	f1=f1/f2;
	__asm("fcvt.s.w fa0,%0" :: "r"(i));
	__asm("lw a3,0(%0)" :: "r"(test));
	__asm("fcvt.w.s %0,fa0,rtz" : "=r"(j));
}

void fmv_test(void)
{
    __asm("sw ra,0(sp)");
    __asm("li a1,0x4135");
    __asm("fcvt.s.w fa0,a1");
#ifdef USE_PSRAM_ADDR
    __asm("li s6,0x108a0000");
#else
    __asm("li s6,0x10015000");
#endif
    __asm("la a4,test1");
    __asm("sw a4,0(s6)");
    __asm("fmv.s	fs0,fa0");
    __asm("lw	a5,0(s6)");
    __asm("fmv.s	fs0,fa0");
    __asm("jalr	a5");

    /*check fs0*/
    __asm("fcvt.w.s	a1,fs0,rtz");
    __asm("li a4,0x4135");
    __asm("beq a1,a4,fmv_ok");
    __asm("j .");

    __asm("fmv_ok:");
    __asm("lw ra,0(sp)");
}

void fsw_test(void)
{
    __asm("addi sp,sp,-32");
    __asm("sw ra,0(sp)");
    __asm("la a5,test1");

    __asm("li a1,0x4135");
    __asm("fcvt.s.w fa0,a1");
#ifdef USE_PSRAM_ADDR
    __asm("li a0,0x108a0000");
#else
    __asm("li a0,0x10015000");
#endif

    __asm("fsw fa0,8(sp)");
    __asm("jalr a5");
    __asm("lw ra,0(sp)");
    __asm("flw fa0,8(sp)");
    __asm("addi sp,sp,32");

    /*check fa0*/
    __asm("fcvt.w.s	a1,fa0,rtz");
    __asm("li a4,0x4135");
    __asm("beq a1,a4,fsw_ok");
    __asm("j .");

    __asm("fsw_ok:");
    __asm("ret");
}

volatile int testbuf[100] = {0};
void ftest(void)
{
    // __asm("lw s6,%0"::"m"(testbuf));
    __asm("sw ra,0(sp)");
    __asm("addi sp,sp,-32");
    __asm("la s6,testbuf");
    __asm("la a4,test1");
    __asm("sw a4,0(s6)");
    __asm("li a5,0x4135");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("fcvt.s.w	fa0,a5");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("la a4,test1");
    __asm("fmv.s fs0,fa0");

    __asm("jal sqrtx");
    __asm("fcvt.w.s	a1,fa0,rtz");
    __asm("lw	a5,0(s6)");
    __asm("lui	a0,0x1019");
    __asm("addi	a0,a0,1064");

    __asm("fmv.s	fs0,fa0");
    __asm("jalr	a5");
    __asm("lw	s3,40(sp)");

    __asm("lw	a5,0(s6)");
    __asm("fsw	fa0,12(sp)");
    __asm("jalr	a5");
    __asm("flw	fa0,12(sp)");

    __asm("lw	a5,0(s6)");
    __asm("flw	fa0,12(sp)");
    __asm("jalr	a5");
    __asm("fsw	fa0,12(sp)");
    __asm("addi sp,sp,32");
    __asm("lw ra,0(sp)");
    __asm("ret");
}

void fpu_test()
{
    fcvt3_test();
    yc_log("fcvt3_test ok\r\n");

    fcvt2_test();
    yc_log("fcvt2_test ok\r\n");

    fcvt_test();
    yc_log("fcvt_test ok\r\n");

    fcvt1_test();
    yc_log("fcvt1_test ok\r\n");

    fmv_test();
    yc_log("fmv_test ok\r\n");

    fsw_test();
    yc_log("fsw_test ok\r\n");

    ftest();
    yc_log("fpu test ok\r\n");
}


/*case1:*/
void test_fnmsub(void)
{
    //__asm("li gp,1");  //modify
    __asm("li a1,8");
    __asm("li a3,0");
    __asm("la a2,valmap1");
    __asm("flw fs0, 0(a2)");
    __asm("flw fa5, 4(a2)");
    __asm("flw fa4, 8(a2)");
    // __asm("nop");
    // __asm("nop");
    __asm("fnmsub.s	fs0,fs0,fa5,fa4"); // fa4 - fs0 * fa5 = 4417f0c7
    __asm("fcvt.s.w	fa4,a1");
    __asm("fdiv.s	fs0,fs0,fa4"); // 4297f0c7
    __asm("beqz	a3,end");
    __asm("j test_fnmsub_fail");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("end:");
    /*check */
    __asm("lw a0,12(a2)");
    __asm("fmv.x.w a1,fs0");
    __asm("bne a0,a1,test_fnmsub_fail");
    __asm("j test_fnmsub_ok");

    __asm("test_fnmsub_fail:");
    yc_log("test_fnmsub error\r\n");

    __asm("valmap1: .word 0x44590000"); // 868
    __asm(".word 0x3f4b470c");          // 0.7940
    __asm(".word 0x44a22000");          // 1297
    __asm(".word 0x4297f0c7");
    __asm("test_fnmsub_ok:");
    yc_log("test_fnmsub ok\r\n");
}

/*case2*/
void test_fnmsub1(void)
{
    __asm("sw ra,0(sp)");
    __asm("addi sp,sp,-80");
    __asm("li s0,229");
    __asm("la a2,valmap2");
    __asm("la a3,test1");
    __asm("flw fa3, 0(a2)");
    __asm("flw fa4, 8(a2)");
    __asm("flw fa2, 16(a2)");
    __asm("flw fa5, 20(a2)");
    __asm("flw fa0, 24(a2)");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("fdiv.s     fa0,fa5,fa0");
    __asm("fmadd.s	fa1,fa3,fa2,fa4"); // 4188fae7
    __asm("fcvt.s.w	fa5,s0");          // fa5 = 0x43650000   //fault point
    __asm("fnmsub.s	fa4,fa3,fa2,fa4"); // 409c1463
    __asm("fsw	fa1,64(sp)");
    __asm("fsw	fa4,72(sp)");
    __asm("fmadd.s	fa1,fa0,fa2,fa5"); // 436d15b9
    __asm("fnmsub.s	fa0,fa0,fa2,fa5"); // 435cea47
    __asm("fsw	fa1,68(sp)");
    __asm("fsw	fa0,76(sp)");
    __asm("jalr   a3");
    __asm("lui a5,0x101b");
    /*check*/
    __asm("fmv.x.w a0,fa5");
    __asm("lw a1,28(a2)");
    __asm("bne a0,a1,test_fnmsub1_fail");

    __asm("lw a0,64(sp)");
    __asm("lw a1,32(a2)");
    __asm("bne a0,a1,test_fnmsub1_fail");

    __asm("lw a0,72(sp)");
    __asm("lw a1,36(a2)");
    __asm("bne a0,a1,test_fnmsub1_fail");

    __asm("lw a0,68(sp)");
    __asm("lw a1,40(a2)");
    __asm("bne a0,a1,test_fnmsub1_fail");

    __asm("lw a0,76(sp)");
    __asm("lw a1,44(a2)");
    __asm("bne a0,a1,test_fnmsub1_fail");

    __asm("addi sp,sp,80");
    __asm("lw ra,0(sp)");
    __asm("j test_fnmsub1_ok");

    __asm("test_fnmsub1_fail:");
    yc_log("test_fnmsub1 error\r\n");

    __asm("valmap2: .word 0x3f1a8ca7"); // nx 0.603708
    __asm(".word 0x3f4c15a2");          // ny 0.7972
    __asm(".word 0x41300000");          // s.x  11
    __asm(".word 0x43650000");          // s.y  229
    __asm(".word 0x41224395");          // search 10.14149

    __asm(".word 0x42ce0000"); // dirY       fa5
    __asm(".word 0x4301338e"); // dirsize    fa0

    __asm(".word 0x43650000");
    __asm(".word 0x4188fae7");
    __asm(".word 0x409c1463");
    __asm(".word 0x436d15b9");
    __asm(".word 0x435cea47");

    __asm("test_fnmsub1_ok:");
    yc_log("test_fnmsub1 ok\r\n");
}

/*case3*/
void test_fmsub(void)
{
    // __asm("addi sp,sp,-80");
    __asm("li s3,0x10000");
    __asm("li s0,0");
    __asm("fcvt.s.w	fa3,s0");
    __asm("li s1,42");
    __asm("fcvt.s.w	fa2,s1");
    __asm("li s2,1");
    __asm("fcvt.s.w	ft2,s2");
    __asm("fmsub.s fa3,fa3,fa2,ft2");
    __asm("fsw fa3,(s3)");

    __asm("li s0,0");
    __asm("fcvt.s.w	fa3,s0");
    __asm("li s1,0");
    __asm("fcvt.s.w	fa2,s1");
    __asm("li s2,147");
    __asm("fcvt.s.w	ft2,s2");
    __asm("li s0,0x10004");
    __asm("fmsub.s fa3,fa3,fa2,ft2");
    __asm("fsw fa3,(s0)");
    __asm("li s1,0x10008");
    __asm("li a0,0xc3130000");
    __asm("sw a0,(s1)");

    /*check*/
    __asm("la a2,valmap3");
    __asm("li s3,0x10000");

    __asm("lw a0,0(s3)");
    __asm("lw a1,0(a2)");
    __asm("bne a0,a1,test_fmsub_fail");

    __asm("lw a0,4(s3)");
    __asm("lw a1,4(a2)");
    __asm("bne a0,a1,test_fmsub_fail");

    __asm("j test_fmsub_ok");

    __asm("test_fmsub_fail:");
    yc_log("test_fmsub error\r\n");

    __asm("valmap3: .word 0xbf800000");
    __asm(".word 0xc3130000");

    __asm("test_fmsub_ok:");
    yc_log("test_fmsub ok\r\n");
}

void test_fmsub1(void)
{
    __asm("li s0,0");
    __asm("fcvt.s.w	fa1,s0");
    __asm("li s1,4");
    __asm("fcvt.s.w	fa2,s1");
    __asm("fcvt.s.w	fa3,s1");
    __asm("li s2,147");
    __asm("fcvt.s.w	ft4,s2");
    __asm("li s0,42");
    __asm("fcvt.s.w	ft1,s0");
    __asm("fcvt.s.w	fa5,s0");
    __asm("li s1,0");
    __asm("fcvt.s.w	ft2,s1");
    __asm("fmsub.s fa1,fa1,fa2,ft4"); //c3130000
    __asm("fmul.s fa2,fa2,ft1");//43280000
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("fmul.s fa2,fa2,ft1");//45dc8000
    __asm("fmul.s fa4,fa4,ft2");
    // __asm("li s0,0x900000");
    // __asm("fsw fa1,(s0)");
    __asm("fmsub.s fa3,fa3,ft2,fa2");//c5dc8000
    __asm("fmsub.s fa5,fa5,ft1,fa4");//44dc8000
    __asm("fmul.s fa2,fa2,ft1");//4890b400

    /*check*/
    __asm("la a2,valmap4");

    __asm("fmv.x.w a0,fa1");
    __asm("lw a1,0(a2)");
    __asm("bne a0,a1,test_fmsub1_fail");

    __asm("fmv.x.w a0,fa2");
    __asm("lw a1,4(a2)");
    __asm("bne a0,a1,test_fmsub1_fail");

    __asm("fmv.x.w a0,fa3");
    __asm("lw a1,8(a2)");
    __asm("bne a0,a1,test_fmsub1_fail");

    __asm("fmv.x.w a0,fa4");
    __asm("lw a1,12(a2)");
    __asm("bne a0,a1,test_fmsub1_fail");

    __asm("fmv.x.w a0,fa5");
    __asm("lw a1,16(a2)");
    __asm("bne a0,a1,test_fmsub1_fail");

    __asm("j test_fmsub1_ok");

    __asm("test_fmsub1_fail:");
    yc_log("test_fmsub1 error\r\n");

    __asm("valmap4: .word 0xc3130000");
    __asm(".word 0x4890b400");
    __asm(".word 0xc5dc8000");
    __asm(".word 0x00000000");
    __asm(".word 0x44dc8000");

    __asm("test_fmsub1_ok:");
    yc_log("test_fmsub1 ok\r\n");
}

void test_fmsub2(void)
{
    __asm("li s0,0");
    __asm("fcvt.s.w	fa1,s0");
    __asm("fcvt.s.w	fa5,s0");
    __asm("fcvt.s.w	fa4,s0");
    __asm("fcvt.s.w	ft1,s0");
    __asm("fcvt.s.w	fa2,s0");
    __asm("li s2,147");
    __asm("fcvt.s.w	ft4,s2");
    __asm("fmsub.s fa1,fa1,fa2,ft4"); //c3130000
    __asm("fmul.s fa2,fa2,ft4");    //todo
    // __asm("fmsub.s fa5,fa5,ft1,fa4");

    /*check*/
    __asm("li a0,0xc3130000");
    __asm("fmv.x.w a1,fa1");
    __asm("bne a0,a1,test_fmsub2_fail");

    __asm("li a0,0x0");
    __asm("fmv.x.w a1,fa2");
    __asm("bne a0,a1,test_fmsub2_fail");

    __asm("j test_fmsub2_ok");

    __asm("test_fmsub2_fail:");
    yc_log("test_fmsub2 error\r\n");

    __asm("test_fmsub2_ok:");
    yc_log("test_fmsub2 ok\r\n");
}

void test_fmsub3(void)
{
    __asm("li s0,0");
    __asm("fcvt.s.w	fa1,s0");
    __asm("fcvt.s.w	fa5,s0");
    __asm("fcvt.s.w	fa4,s0");
    __asm("fcvt.s.w	ft1,s0");
    __asm("fcvt.s.w	fa2,s0");
    __asm("li s2,147");
    __asm("fcvt.s.w	ft4,s2");
    __asm("fmsub.s fa1,fa1,fa2,ft4");
    // __asm("fmul.s fa2,fa2,ft4");    //todo
    __asm("fmsub.s fa5,fa5,ft1,fa4");

    /*check*/
    __asm("li a0,0xc3130000");
    __asm("fmv.x.w a1,fa1");
    __asm("bne a0,a1,test_fmsub3_fail");

    __asm("li a0,0x0");
    __asm("fmv.x.w a1,fa5");
    __asm("bne a0,a1,test_fmsub3_fail");

    __asm("j test_fmsub3_ok");

    __asm("test_fmsub3_fail:");
    yc_log("test_fmsub3 error\r\n");

    __asm("test_fmsub3_ok:");
    yc_log("test_fmsub3 ok\r\n");
}

void test_fmadd1(void)
{
    __asm("addi sp,sp,-200");
    __asm("li s0,42");
    __asm("fcvt.s.w	ft8,s0");
    __asm("fcvt.s.w	fa1,s0");
    __asm("fcvt.s.w	fa4,s0");
    __asm("li s1,1");
    __asm("fcvt.s.w	ft1,s1");
    __asm("li s2,0");
    __asm("fcvt.s.w	ft10,s2");
    __asm("fmadd.s ft10,ft8,ft1,ft10"); //fault mul add result=0
    __asm("flw fa1,-4(sp)");
    __asm("flw fa4,-8(sp)");

    /*check*/
    __asm("li a0,0x42280000");
    __asm("fmv.x.w a1,ft10");
    __asm("bne a0,a1,test_fmadd1_fail");

    __asm("addi sp,sp,200");
    __asm("j test_fmadd1_ok");

    __asm("test_fmadd1_fail:");
    yc_log("test_fmadd1 error\r\n");

    __asm("test_fmadd1_ok:");
    yc_log("test_fmadd1 ok\r\n");
}

void test_fmadd2(void)
{
    __asm("li s0,42");
    // __asm("sw s0,140(sp)");
    // __asm("sw s0,144(sp)");
    __asm("fcvt.s.w  ft8,s0");
    __asm("li s1,1");
    __asm("fcvt.s.w  ft1,s1");
    __asm("li s2,0");
    __asm("fcvt.s.w  ft10,s2");
    __asm("fmadd.s ft10,ft8,ft1,ft10");
    __asm("flw fa1,140(sp)");
    __asm("flw fa4,144(sp)");

    /*check*/
    __asm("li a0,0x42280000");
    __asm("fmv.x.w a1,ft10");
    __asm("bne a0,a1,test_fmadd2_fail");

    __asm("j test_fmadd2_ok");

    __asm("test_fmadd2_fail:");
    yc_log("test_fmadd2 error\r\n");


    __asm("test_fmadd2_ok:");
    yc_log("test_fmadd2 ok\r\n");
}


typedef struct
{
    float a11;
    float a12;
    float a13;
    float a21;
    float a22;
    float a23;
    float a31;
    float a32;
    float a33;
} PerspectiveTransform_t1;

static void init(PerspectiveTransform_t1 *one, float b11, float b21, float b31,
                 float b12, float b22, float b32,
                 float b13, float b23, float b33)
{
    one->a11 = b11;
    one->a12 = b12;
    one->a13 = b13;
    one->a21 = b21;
    one->a22 = b22;
    one->a23 = b23;
    one->a31 = b31;
    one->a32 = b32;
    one->a33 = b33;
}
void build_adjoint_t1(void)
{
	PerspectiveTransform_t1 resone =
	{
		.a11 = 42.0, // 0x42280000
		.a12 = 0.0,  // 0x00000000
		.a13 = 0.0,  // 0x00000000
		.a21 = 0.0,  // 0x00000000
		.a22 = 42.0, // 0x42280000
		.a23 = 0.0,  // 0x00000000
		.a31 = -147,  // 0xc3130000
		.a32 = -147,  // 0xc3130000
		.a33 = 1764,  // 0x44dc8000
	};
	PerspectiveTransform_t1 one =
	{
		.a11 = 42.0, // 0x42280000
		.a12 = 0.0,  // 0x00000000
		.a13 = 0.0,  // 0x00000000
		.a21 = 0.0,  // 0x00000000
		.a22 = 42.0, // 0x42280000
		.a23 = 0.0,  // 0x00000000
		.a31 = 3.5,  // 0x40600000
		.a32 = 3.5,  // 0x40600000
		.a33 = 1.0,  // 0x3f800000
	};
	PerspectiveTransform_t1 newone;
	init(&newone,
			one.a22 * one.a33 - one.a23 * one.a32,
			one.a23 * one.a31 - one.a21 * one.a33,
			one.a21 * one.a32 - one.a22 * one.a31,

			one.a13 * one.a32 - one.a12 * one.a33,
			one.a11 * one.a33 - one.a13 * one.a31,
			one.a12 * one.a31 - one.a11 * one.a32,

			one.a12 * one.a23 - one.a13 * one.a22,
			one.a13 * one.a21 - one.a11 * one.a23,
			one.a11 * one.a22 - one.a12 * one.a21);

	// printv((uint8_t *)&one, 4 * 9, "one");
	// printv((uint8_t *)&newone, 4 * 9, "newone");
	// printv((uint8_t *)&resone, 4 * 9, "resone");
	if (memcmp(&newone, &resone, sizeof(resone)) != 0)
	{
		yc_log("build_adjoint_t1 err\n");
	}
	yc_log("build_adjoint_t1 ok\n");
}

void timers_t1(void)
{
	PerspectiveTransform_t1 newone;
	PerspectiveTransform_t1 resone =
	{
		.a11 = 3276,  // 0x454cc000
		.a12 = 4326, // 0x45873000
		.a13 = 0.0,   // 0x00000000
		.a21 = -3654, // 0xc5646000
		.a22 = 4116,  // 0x4580a000
		.a23 = 0.0,   // 0x00000000
		.a31 = 184779, // 0x483472c0
		.a32 = 189189, // 0x4838c140
		.a33 = 1764,   // 0x44dc8000
	};
	PerspectiveTransform_t1 one =
	{
		.a11 = 78.0,  // 0x429c0000
		.a12 = 103.0, // 0x42ce0000
		.a13 = 0.0,   // 0x00000000
		.a21 = -87.0, // 0xc2ae0000
		.a22 = 98.0,  // 0x42c40000
		.a23 = 0.0,   // 0x00000000
		.a31 = 104.0, // 0x42d00000
		.a32 = 124.0, // 0x42f80000
		.a33 = 1.0,   // 0x3f800000
	};
	PerspectiveTransform_t1 other =
	{
		.a11 = 42.0,   // 0x42280000
		.a12 = 0.0,    // 0x00000000
		.a13 = 0.0,    // 0x00000000
		.a21 = 0.0,    // 0x00000000
		.a22 = 42.0,   // 0x42280000
		.a23 = 0.0,    // 0x00000000
		.a31 = -147.0, // 0xc3130000
		.a32 = -147.0, // 0xc3130000
		.a33 = 1764.0, // 0x44dc8000
	};
	init(&newone,
			one.a11 * other.a11 + one.a21 * other.a12 + one.a31 * other.a13,
			one.a11 * other.a21 + one.a21 * other.a22 + one.a31 * other.a23,
			one.a11 * other.a31 + one.a21 * other.a32 + one.a31 * other.a33,

			one.a12 * other.a11 + one.a22 * other.a12 + one.a32 * other.a13,
			one.a12 * other.a21 + one.a22 * other.a22 + one.a32 * other.a23,
			one.a12 * other.a31 + one.a22 * other.a32 + one.a32 * other.a33,

			one.a13 * other.a11 + one.a23 * other.a12 + one.a33 * other.a13,
			one.a13 * other.a21 + one.a23 * other.a22 + one.a33 * other.a23,
			one.a13 * other.a31 + one.a23 * other.a32 + one.a33 * other.a33);
	// printv((uint8_t *)&one, 4 * 9, "one");
	// printv((uint8_t *)&other, 4 * 9, "other");
	// printv((uint8_t *)&resone, 4 * 9, "resone");
	// printv((uint8_t *)&newone, 4 * 9, "newone");
	if (memcmp(&newone, &resone, sizeof(resone)) != 0)
	{
		yc_log("timers_t1 err\n");
	}
	yc_log("timers_t1 ok\n");
}


void transform_t1(void)
{
	int count = 8;
	float res[8] =
	{
		104,//0x42d00000
		124,//0x42f80000
		104,//0x42d00000
		124,//0x42f80000
		104,//0x42d00000
		124,//0x42f80000
		104,//0x42d00000
		124,//0x42f80000
	};
	PerspectiveTransform_t1 t_one =
	{
		// .a11 = 3276,   // 0x454cc000,
		// .a12 = 4326,   // 0x45873000,
		// .a13 = 0,      // 0x00000000
		// .a21 = -3654,  // 0xc5646000,
		// .a22 = 4116,   // 0x4580a000,
		// .a23 = 0,      // 0x00000000
		// .a31 = 184779, // 0x483472c0,
		// .a32 = 189189, // 0x4838c140,
		// .a33 = 1764,   // 0x44dc8000,

		.a11 = -0.0,     // 0x80000000
		.a12 = 0.0,      // 0x00000000
		.a13 = 0.0,      // 0x00000000
		.a21 = 0.0,      // 0x00000000
		.a22 = 0.0,      // 0x00000000
		.a23 = 0.0,      // 0x00000000
		.a31 = 183456.0, // 0x48332800
		.a32 = 218736.0, // 0x48559c00
		.a33 = 1764.0,   // 0x44dc8000
	};
	PerspectiveTransform_t1 *one = &t_one;
	float points[8] =
	{
		0.5, // 0x3f000000
		0.5, // 0x3f000000
		1.5, // 0x3f400000
		0.5, // 0x3f000000
		2.5, // 40200000
		0.5, // 0x3f000000
		3.5, // 40600000
		0.5, // 0x3f000000

	};
	for (int i = 0; i < count; i += 2)
	{
		float x = points[i];
		float y = points[i + 1];
		float denominator = one->a13 * x + one->a23 * y + one->a33;
		points[i] = (one->a11 * x + one->a21 * y + one->a31) / denominator;
		points[i + 1] = (one->a12 * x + one->a22 * y + one->a32) / denominator;
	}
	// printv((uint8_t *)res, 4 * 8, "res");
	// printv((uint8_t *)points, 4 * 8, "points");
	if (memcmp(points, res, sizeof(res)) != 0)
	{
		yc_log("transform_t1 err\n");
	}
	yc_log("transform_t1 ok\n");
}


void fpu_test1()
{
    test_fnmsub();
    test_fnmsub1();
    test_fmsub();
    test_fmsub1();
    test_fmsub2();
    test_fmsub3();
    test_fmadd1();
    test_fmadd2();

    build_adjoint_t1();
    timers_t1();
    transform_t1();
}