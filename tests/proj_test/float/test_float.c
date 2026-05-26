#include "test_float.h"

int float_test();
// int fpu_test();

float sqrtx(float b)
{
        float x=1;int step=0;
        while((x*x-b<-0.000001||x*x-b>0.000001)&&step<50) {
                x=(b/x+x)/2.0;step++;
        }
        return x;
}

void test_float(void)
{
	// float a = 221.23567, b = 7.189762, c = 88.9872;
	// volatile float d;
	// int i;

	// __asm("fmadd.s %0,%1,%2,%3" : "=f"(d) : "f"(a), "f"(b), "f"(c));
	// yc_log("a = %0.5f, b = %0.5f, c = %0.5f, d = %0.5f\n", a, b, c, d);
	// // return;
	// __asm("fsub.s %0,%1,%2" : "=f"(c) : "f"(a), "f"(b));
	// __asm("fmul.s %0,%1,%2" : "=f"(d) : "f"(a), "f"(b));
	// __asm("fadd.s %0,%1,%2" : "=f"(d) : "f"(c), "f"(d));
	// __asm("fadd.s %0,%1,%2" : "=f"(c) : "f"(a), "f"(c));
	// __asm("fdiv.s %0,%1,%2" : "=f"(c) : "f"(a), "f"(b));
	// yc_log("a= %x, b = %x\n", *(unsigned int*)&a, *(unsigned int*)&b);

	// yc_log("a = %0.5f, sqrt = %0.5f\n", a, sqrtx(a));
	// __asm("fdiv.s %0,%1,%2" : "=f"(c) : "f"(a), "f"(b));
	// __asm("fsqrt.s %0,%1" : "=f"(d) : "f"(a));
	// yc_log("a= %x, b = %x, c= %x, d= %x\n", *(unsigned int*)&a, *(unsigned int*)&b, *(unsigned int*)&c, *(unsigned int*)&d);
	// yc_log("a = %0.5f, b = %0.5f, c = %0.5f, d = %0.5f, \n", a, b, c, d);

	// for(i = 0;i < 5;i++) {
	// 	yc_log("a= %x, b = %x\n", *(unsigned int*)&a, *(unsigned int*)&b);
	// 	a = a*b - b;
	// 	yc_log("a= %x\n", *(unsigned int*)&a);
	// }
    __asm("addi sp,sp,-36");

    __asm("sw	a1,4(sp)");
    __asm("sw	a2,8(sp)");
    __asm("sw	a3,12(sp)");
    __asm("sw	a4,16(sp)");
    __asm("sw	a5,20(sp)");
    __asm("sw	a6,24(sp)");
    __asm("sw	a7,28(sp)");
    __asm("sw gp,32(sp)");
    int ret = float_test();
    __asm("lw	a1,4(sp)");
    __asm("lw	a2,8(sp)");
    __asm("lw	a3,12(sp)");
    __asm("lw	a4,16(sp)");
    __asm("lw	a5,20(sp)");
    __asm("lw	a6,24(sp)");
    __asm("lw	a7,28(sp)");
    __asm("lw gp,32(sp)");
    __asm("addi sp,sp,36");

    if(0 != ret)
    {
        yc_log("float_test.S test error[%d]\n",ret);
    }
    else
    {
        yc_log("float_test.S test success\n");
    }
}

void fmv_w_x_test(void)
{
    float a;
    int   b = 0x40e61288; //7.189762
    float c;
    yc_log("fmv_w_x_test\r\n");
    __asm volatile ("fmv.w.x %0,%1" :"=f"(a) :"r"(b));
    // yc_log("c=%0.7f - %x\n",a,*(uint32_t *)&a);

    __asm volatile ("fmv.x.w %0,%1" :"=r"(b) :"f"(a));
    // yc_log("c=%x - %0.7f\n",b,*(float *)&b);
}

/*
    f(n)madd/f(n)msub       NV OF UF NX
    fadd/fsub/fmul/         NV OF UF NX
    fdiv                    NV OF UF NX DZ
    fmin/fmax               NV
    fcvt.w(u).s             NV NX
    feq/flt/fle             NV NX
    *fsqrt                  NV NX
    fcvt.s.w(u)             NX
    -----------------------------------------
    0.result is QNnN when NV
    1.mul
    */
#define NV_TEST_OP2(testnum,inst,t)   {                            \
    __asm("addi sp,sp,-36");\
    __asm("sw	a1,4(sp)");\
    __asm("sw	a2,8(sp)");\
    __asm("sw	a3,12(sp)");\
    __asm("sw	a4,16(sp)");\
    __asm("sw	a5,20(sp)");\
    __asm("sw	a6,24(sp)");\
    __asm("sw	a7,28(sp)");\
    __asm("sw gp,32(sp)");\
    __asm("addi sp,sp,-100");\
    volatile float a,b,c,d;                                        \
    int flag;                                                      \
    __asm("li gp,"#testnum);                                       \
    for(int y = 0;y<2;y++)                                         \
    {                                                              \
        for(int i = 0;i<sizeof(t)/sizeof(int);i++)                 \
        {                                                          \
            a=b=2.5;                                               \
            if(y==0) a = *(float*)(t+i);                           \
            if(y==1) b = *(float*)(t+i);                           \
            __asm volatile(#inst " %0,%1,%2":"=f"(d):"f"(a),"f"(b)); \
            __asm volatile("fsflags  %0,x0" : "=r"(flag):"r"(flag)); \
        }                                                          \
    }                                                              \
    __asm("addi sp,sp,100");\
    __asm("lw	a1,4(sp)");\
    __asm("lw	a2,8(sp)");\
    __asm("lw	a3,12(sp)");\
    __asm("lw	a4,16(sp)");\
    __asm("lw	a5,20(sp)");\
    __asm("lw	a6,24(sp)");\
    __asm("lw	a7,28(sp)");\
    __asm("lw gp,32(sp)");\
    __asm("addi sp,sp,36");\
    yc_log("***fflags "#inst" OK*******\r\n");                   \
}

#define NV_TEST_OP3(testnum,inst,t)   {                            \
    __asm("addi sp,sp,-36");\
    __asm("sw	a1,4(sp)");\
    __asm("sw	a2,8(sp)");\
    __asm("sw	a3,12(sp)");\
    __asm("sw	a4,16(sp)");\
    __asm("sw	a5,20(sp)");\
    __asm("sw	a6,24(sp)");\
    __asm("sw	a7,28(sp)");\
    __asm("sw gp,32(sp)");\
    __asm("addi sp,sp,-100");\
    volatile float a,b,c,d;                                        \
    int flag;                                                      \
    __asm("li gp,"#testnum);                                         \
    for(int y = 0;y<2;y++)                                         \
    {                                                              \
        for(int i = 0;i<sizeof(t)/sizeof(int);i++)                 \
        {                                                          \
            a=b=1.0;c=2.0;                                         \
            if(y==0) a = *(float*)(t+i);                           \
            if(y==1) b = *(float*)(t+i);                           \
            if(y==2) c = *(float*)(t+i);                           \
            __asm volatile(#inst " %0,%1,%2,%3":"=f"(d):"f"(a),"f"(b),"f"(c)); \
            __asm volatile("fsflags  %0,x0" : "=r"(flag):"r"(flag)); \
        }                                                          \
    }                                                              \
    __asm("addi sp,sp,100");\
    __asm("lw	a1,4(sp)");\
    __asm("lw	a2,8(sp)");\
    __asm("lw	a3,12(sp)");\
    __asm("lw	a4,16(sp)");\
    __asm("lw	a5,20(sp)");\
    __asm("lw	a6,24(sp)");\
    __asm("lw	a7,28(sp)");\
    __asm("lw gp,32(sp)");\
    __asm("addi sp,sp,36");\
    yc_log("***fflags "#inst" OK*******\r\n");                   \
}

#define NV_TEST_CMP(testnum,inst,t)   {                            \
    __asm("addi sp,sp,-36");\
    __asm("sw	a1,4(sp)");\
    __asm("sw	a2,8(sp)");\
    __asm("sw	a3,12(sp)");\
    __asm("sw	a4,16(sp)");\
    __asm("sw	a5,20(sp)");\
    __asm("sw	a6,24(sp)");\
    __asm("sw	a7,28(sp)");\
    __asm("sw gp,32(sp)");\
    __asm("addi sp,sp,-100");\
    volatile float a,b,c,d;                                        \
    int flag;                                                      \
    int e;                                                         \
    __asm("li gp,"#testnum);                                         \
    for(int y = 0;y<2;y++)                                         \
    {                                                              \
        for(int i = 0;i<sizeof(t)/sizeof(int);i++)                 \
        {                                                          \
            a=b=1.0;                                               \
            if(y==0) a = *(float*)(t+i);                           \
            if(y==1) b = *(float*)(t+i);                           \
            __asm volatile(#inst " %0,%1,%2":"=r"(e):"f"(a),"f"(b)); \
            __asm volatile("fsflags  %0,x0" : "=r"(flag):"r"(flag)); \
            \
        }                                                          \
    }                                                              \
    __asm("addi sp,sp,100");\
    __asm("lw	a1,4(sp)");\
    __asm("lw	a2,8(sp)");\
    __asm("lw	a3,12(sp)");\
    __asm("lw	a4,16(sp)");\
    __asm("lw	a5,20(sp)");\
    __asm("lw	a6,24(sp)");\
    __asm("lw	a7,28(sp)");\
    __asm("lw gp,32(sp)");\
    __asm("addi sp,sp,36");\
    yc_log("***fflags "#inst" OK*******\r\n");                   \
}                                                                \

void fflags_nv_test(void)
{            /*-NaN        NaN        -Inf       Inf        QNaN     SNaN*/
    // int t[] ={0xffffffff,0x7fffffff,0xff800000,0x7f800000,0x7fc00000,0x7f800001,0x7e38cba9};
    int t[] ={0xffffffff,0x7fffffff,0x7fc00000,0x7f800001,0x7e38cba9};
    // volatile float a,b,c,d;
    // int flag;

    NV_TEST_OP2(0x100,fmul.s,t)
    NV_TEST_OP2(0x101,fadd.s,t)
    NV_TEST_OP2(0x102,fsub.s,t)
    NV_TEST_OP3(0x103,fmadd.s,t)
    NV_TEST_OP3(0x104,fmsub.s,t)
    NV_TEST_OP3(0x105,fnmsub.s,t)
    NV_TEST_OP3(0x106,fnmadd.s,t)

    /*fmin/fmax
        1)if both inputs are NaN,the result is the canonical Nan
        2)if only one operand is a NaN, the result is the not-Nan(eg. fmin(NaN,1)=1)
        3)Signaling NaN inputs set the invalid operation exception flag,even when the result is not NaN
    see float_test.s (fmin/fmax test)
    */

    /*flt/fle/feq
        1)if either input is a NaN, the result is 0
        2)flt/fle set the invalid operation exception falg if either input is a NaN,
        3)it only sets the invalid operation exception flag if either input is a Signaling NaN
    see float_test.s (flt/fle/feq test)
    */

    /*fdiv
        1)it only sets the invalid operation exception flag if either input is a Signaling NaN
    */
}



void fflags_div0_test(void)
{
    int r;
    int r1=0;
    volatile float a=0;
    volatile float b=0;
    volatile float c=0;

    a = 6.3;
    b = 2.0;
    c = a/b;
    // yc_log("6.3/2.0=%0.5f\n",c);

    b= 0;
    c = a/b;
    __asm("fsflags  %0,x0" :: "r"(r));
    yc_log("div 0=%x\n",c);
    yc_log("fflags %s fflags = %x\r\n",(r&8) ? "OK":"err",r);

}




void fflags_test(void)
{


    /*NV:Invalid Operation
    0:ICVT exp > MAX
    1:add  NaN result
    2:mul
    */
    fflags_nv_test();


    /*DZ:Divide by zero
    0: div
    */
    fflags_div0_test();

    /*OF:Overflow*/
    /*UF:Underflow*/
    /*NX:Inexact*/
}



void test_float1(void)
{
    yc_log("float test\n");
    /*step1:*/
    test_float();

    /*step2:*/
    fflags_test();

    /*step3:*/
    fmv_w_x_test();

    /*step4:*/
    fpu_test();

    /*step5:*/
    fpu_test1();

}