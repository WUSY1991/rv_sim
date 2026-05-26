#define MASK_XLEN(x) ((x) & ((1 << (__riscv_xlen - 1) << 1) - 1))
/////////////////////origin funct//////////////////////////////////////////
#define TEST_CASE( testnum, testreg, correctval, code... ) \
test_ ## testnum: \
    code; \
    li  x29, MASK_XLEN(correctval); \
    li  TESTNUM, testnum; \
    bne testreg, x29, fail;

// Dzj: Newly added specially for rv32ud
#define TEST_CASE_fclass( testnum, testreg, correctval,input, code... ) \
test_ ## testnum: \
 la  a0, test_ ## testnum ## _data ;\
    code; \
    li  x29, MASK_XLEN(correctval); \
    li  TESTNUM, testnum; \
    bne testreg, x29, fail;\
    .pushsection .data; \
    .align 3; \
    test_ ## testnum ## _data: \
    .dword input; \
    .popsection

// Dzj: Newly added specially for rv32ud
#define TEST_CASE_fcvt( testnum, testreg, correctval_high, correctval_low, code... ) \
test_ ## testnum: \
    code; \
    li a6 , correctval_high; \
    li a5 , correctval_low; \
    li  TESTNUM, testnum; \
    bne a3, a5, fail;\
    bne a4, a6, fail;
//
#define TEST_CASE_fcvt_w( testnum, testreg, correctval_high , correctval_low , code... ) \
test_ ## testnum: \
    code; \
    li a5 , correctval_low; \
    li  TESTNUM, testnum; \
    bne testreg, a5, fail;
//
#define TEST_CASE_ldst( testnum, testreg, correctval_high , correctval_low , code... ) \
test_ ## testnum: \
    code; \
    li a6 , correctval_high; \
    li a5 , correctval_low; \
    li  TESTNUM, testnum; \
    bne a3, a5, fail;\
    bne a4, a6, fail;
//
#define TEST_CASE_FSGNJD( testnum, testreg, correctval_high, correctval_low, code... ) \
test_ ## testnum: \
    code; \
    li a6 , correctval_high; \
    li a5 , correctval_low; \
    li  TESTNUM, testnum; \
    bne a3, a5, fail;\
    bne a4, a6, fail;
//
#define TEST_CASE_1_FSGNJS( testnum, testreg, correctval, code... ) \
test_ ## testnum: \
    code; \
    li  x29, MASK_XLEN(correctval); \
    li  TESTNUM, testnum; \
    bne testreg, x29, fail;
//
#define TEST_CASE_2_FSGNJS( testnum, testreg, correctval_high, correctval_low, code... ) \
test_ ## testnum: \
    code; \
    li a6 , correctval_high; \
    li a5 , correctval_low; \
    li  TESTNUM, testnum; \
    bne a3, a5, fail;\
    bne a4, a6, fail;
//
#define TEST_CASE_1_FSGNJD_SP( testnum, testreg, correctval, code... ) \
test_ ## testnum: \
    code; \
    li  x29, MASK_XLEN(correctval); \
    li  TESTNUM, testnum; \
    bne testreg, x29, fail;
//
#define TEST_CASE_2_FSGNJD_SP( testnum, testreg, correctval_high, correctval_low, code... ) \
test_ ## testnum: \
    code; \
    li a6 , correctval_high; \
    li a5 , correctval_low; \
    li  TESTNUM, testnum; \
    bne a3, a5, fail;\
    bne a4, a6, fail;
/////////////////////////////////////////////////////////////////////////////////////

# We use a macro hack to simpify code generation for various numbers
# of bubble cycles.

#define TEST_INSERT_NOPS_0
#define TEST_INSERT_NOPS_1  nop; TEST_INSERT_NOPS_0
#define TEST_INSERT_NOPS_2  nop; TEST_INSERT_NOPS_1
#define TEST_INSERT_NOPS_3  nop; TEST_INSERT_NOPS_2
#define TEST_INSERT_NOPS_4  nop; TEST_INSERT_NOPS_3
#define TEST_INSERT_NOPS_5  nop; TEST_INSERT_NOPS_4
#define TEST_INSERT_NOPS_6  nop; TEST_INSERT_NOPS_5
#define TEST_INSERT_NOPS_7  nop; TEST_INSERT_NOPS_6
#define TEST_INSERT_NOPS_8  nop; TEST_INSERT_NOPS_7
#define TEST_INSERT_NOPS_9  nop; TEST_INSERT_NOPS_8
#define TEST_INSERT_NOPS_10 nop; TEST_INSERT_NOPS_9


#-----------------------------------------------------------------------
# RV64UI MACROS
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Tests for instructions with immediate operand
#-----------------------------------------------------------------------

#define SEXT_IMM(x) ((x) | (-(((x) >> 11) & 1) << 11))

#define TEST_IMM_OP( testnum, inst, result, val1, imm ) \
    TEST_CASE( testnum, x30, result, \
      li  x1, MASK_XLEN(val1); \
      inst x30, x1, SEXT_IMM(imm); \
    )

#define TEST_IMM_SRC1_EQ_DEST( testnum, inst, result, val1, imm ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, MASK_XLEN(val1); \
      inst x1, x1, SEXT_IMM(imm); \
    )

#define TEST_IMM_DEST_BYPASS( testnum, nop_cycles, inst, result, val1, imm ) \
    TEST_CASE( testnum, x6, result, \
      li  x4, 0; \
1:    li  x1, MASK_XLEN(val1); \
      inst x30, x1, SEXT_IMM(imm); \
      TEST_INSERT_NOPS_ ## nop_cycles \
      addi  x6, x30, 0; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_IMM_SRC1_BYPASS( testnum, nop_cycles, inst, result, val1, imm ) \
    TEST_CASE( testnum, x30, result, \
      li  x4, 0; \
1:    li  x1, MASK_XLEN(val1); \
      TEST_INSERT_NOPS_ ## nop_cycles \
      inst x30, x1, SEXT_IMM(imm); \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_IMM_ZEROSRC1( testnum, inst, result, imm ) \
    TEST_CASE( testnum, x1, result, \
      inst x1, x0, SEXT_IMM(imm); \
    )

#define TEST_IMM_ZERODEST( testnum, inst, val1, imm ) \
    TEST_CASE( testnum, x0, 0, \
      li  x1, MASK_XLEN(val1); \
      inst x0, x1, SEXT_IMM(imm); \
    )

#-----------------------------------------------------------------------
# Tests for an instruction with register operands
#-----------------------------------------------------------------------

#define TEST_R_OP( testnum, inst, result, val1 ) \
    TEST_CASE( testnum, x30, result, \
      li  x1, val1; \
      inst x30, x1; \
    )

#define TEST_R_SRC1_EQ_DEST( testnum, inst, result, val1 ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, val1; \
      inst x1, x1; \
    )

#define TEST_R_DEST_BYPASS( testnum, nop_cycles, inst, result, val1 ) \
    TEST_CASE( testnum, x6, result, \
      li  x4, 0; \
1:    li  x1, val1; \
      inst x30, x1; \
      TEST_INSERT_NOPS_ ## nop_cycles \
      addi  x6, x30, 0; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#-----------------------------------------------------------------------
# Tests for an instruction with register-register operands
#-----------------------------------------------------------------------

#define TEST_RR_OP( testnum, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x30, result, \
      li  x1, MASK_XLEN(val1); \
      li  x2, MASK_XLEN(val2); \
      inst x30, x1, x2; \
    )

#define TEST_RR_SRC1_EQ_DEST( testnum, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, MASK_XLEN(val1); \
      li  x2, MASK_XLEN(val2); \
      inst x1, x1, x2; \
    )

#define TEST_RR_SRC2_EQ_DEST( testnum, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x2, result, \
      li  x1, MASK_XLEN(val1); \
      li  x2, MASK_XLEN(val2); \
      inst x2, x1, x2; \
    )

#define TEST_RR_SRC12_EQ_DEST( testnum, inst, result, val1 ) \
    TEST_CASE( testnum, x1, result, \
      li  x1, MASK_XLEN(val1); \
      inst x1, x1, x1; \
    )

#define TEST_RR_DEST_BYPASS( testnum, nop_cycles, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x6, result, \
      li  x4, 0; \
1:    li  x1, MASK_XLEN(val1); \
      li  x2, MASK_XLEN(val2); \
      inst x30, x1, x2; \
      TEST_INSERT_NOPS_ ## nop_cycles \
      addi  x6, x30, 0; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_RR_SRC12_BYPASS( testnum, src1_nops, src2_nops, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x30, result, \
      li  x4, 0; \
1:    li  x1, MASK_XLEN(val1); \
      TEST_INSERT_NOPS_ ## src1_nops \
      li  x2, MASK_XLEN(val2); \
      TEST_INSERT_NOPS_ ## src2_nops \
      inst x30, x1, x2; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_RR_SRC21_BYPASS( testnum, src1_nops, src2_nops, inst, result, val1, val2 ) \
    TEST_CASE( testnum, x30, result, \
      li  x4, 0; \
1:    li  x2, MASK_XLEN(val2); \
      TEST_INSERT_NOPS_ ## src1_nops \
      li  x1, MASK_XLEN(val1); \
      TEST_INSERT_NOPS_ ## src2_nops \
      inst x30, x1, x2; \
      addi  x4, x4, 1; \
      li  x5, 2; \
      bne x4, x5, 1b \
    )

#define TEST_RR_ZEROSRC1( testnum, inst, result, val ) \
    TEST_CASE( testnum, x2, result, \
      li x1, MASK_XLEN(val); \
      inst x2, x0, x1; \
    )

#define TEST_RR_ZEROSRC2( testnum, inst, result, val ) \
    TEST_CASE( testnum, x2, result, \
      li x1, MASK_XLEN(val); \
      inst x2, x1, x0; \
    )

#define TEST_RR_ZEROSRC12( testnum, inst, result ) \
    TEST_CASE( testnum, x1, result, \
      inst x1, x0, x0; \
    )

#define TEST_RR_ZERODEST( testnum, inst, val1, val2 ) \
    TEST_CASE( testnum, x0, 0, \
      li x1, MASK_XLEN(val1); \
      li x2, MASK_XLEN(val2); \
      inst x0, x1, x2; \
    )

#-----------------------------------------------------------------------
# Test memory instructions
#-----------------------------------------------------------------------

#define TEST_LD_OP( testnum, inst, result, offset, base ) \
    TEST_CASE( testnum, x30, result, \
      la  x1, base; \
      inst x30, offset(x1); \
    )

#define TEST_ST_OP( testnum, load_inst, store_inst, result, offset, base ) \
    TEST_CASE( testnum, x30, result, \
      la  x1, base; \
      li  x2, result; \
      store_inst x2, offset(x1); \
      load_inst x30, offset(x1); \
    )

#define TEST_LD_DEST_BYPASS( testnum, nop_cycles, inst, result, offset, base ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  la  x1, base; \
    inst x30, offset(x1); \
    TEST_INSERT_NOPS_ ## nop_cycles \
    addi  x6, x30, 0; \
    li  x29, result; \
    bne x6, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b; \

#define TEST_LD_SRC1_BYPASS( testnum, nop_cycles, inst, result, offset, base ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  la  x1, base; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x30, offset(x1); \
    li  x29, result; \
    bne x30, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_ST_SRC12_BYPASS( testnum, src1_nops, src2_nops, load_inst, store_inst, result, offset, base ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  li  x1, result; \
    TEST_INSERT_NOPS_ ## src1_nops \
    la  x2, base; \
    TEST_INSERT_NOPS_ ## src2_nops \
    store_inst x1, offset(x2); \
    load_inst x30, offset(x2); \
    li  x29, result; \
    bne x30, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_ST_SRC21_BYPASS( testnum, src1_nops, src2_nops, load_inst, store_inst, result, offset, base ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  la  x2, base; \
    TEST_INSERT_NOPS_ ## src1_nops \
    li  x1, result; \
    TEST_INSERT_NOPS_ ## src2_nops \
    store_inst x1, offset(x2); \
    load_inst x30, offset(x2); \
    li  x29, result; \
    bne x30, x29, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_BR2_OP_TAKEN( testnum, inst, val1, val2 ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x1, val1; \
    li  x2, val2; \
    inst x1, x2, 2f; \
    bne x0, TESTNUM, fail; \
1:  bne x0, TESTNUM, 3f; \
2:  inst x1, x2, 1b; \
    bne x0, TESTNUM, fail; \
3:

#define TEST_BR2_OP_NOTTAKEN( testnum, inst, val1, val2 ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x1, val1; \
    li  x2, val2; \
    inst x1, x2, 1f; \
    bne x0, TESTNUM, 2f; \
1:  bne x0, TESTNUM, fail; \
2:  inst x1, x2, 1b; \
3:

#define TEST_BR2_SRC12_BYPASS( testnum, src1_nops, src2_nops, inst, val1, val2 ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  li  x1, val1; \
    TEST_INSERT_NOPS_ ## src1_nops \
    li  x2, val2; \
    TEST_INSERT_NOPS_ ## src2_nops \
    inst x1, x2, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_BR2_SRC21_BYPASS( testnum, src1_nops, src2_nops, inst, val1, val2 ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  li  x2, val2; \
    TEST_INSERT_NOPS_ ## src1_nops \
    li  x1, val1; \
    TEST_INSERT_NOPS_ ## src2_nops \
    inst x1, x2, fail; \
    addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#-----------------------------------------------------------------------
# Test jump instructions
#-----------------------------------------------------------------------

#define TEST_JR_SRC1_BYPASS( testnum, nop_cycles, inst ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  la  x6, 2f; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x6; \
    bne x0, TESTNUM, fail; \
2:  addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \

#define TEST_JALR_SRC1_BYPASS( testnum, nop_cycles, inst ) \
test_ ## testnum: \
    li  TESTNUM, testnum; \
    li  x4, 0; \
1:  la  x6, 2f; \
    TEST_INSERT_NOPS_ ## nop_cycles \
    inst x19, x6, 0; \
    bne x0, TESTNUM, fail; \
2:  addi  x4, x4, 1; \
    li  x5, 2; \
    bne x4, x5, 1b \


#-----------------------------------------------------------------------
# RV64UF MACROS
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Tests floating-point instructions
#-----------------------------------------------------------------------
#define OUTRf 0f:7e38cba9
#define nqNaNf 0f:ffc00000
#define qNaNf 0f:7fc00000
#define sNaNf 0f:7f800001
#define qNaN 0d:7ff8000000000000
#define sNaN 0d:7ff0000000000001
///////////////////////////origin funct//////////////////////////////////////////////////////////
#define TEST_FP_OP_S_INTERNAL( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  flw f0, 0(a0); \
  flw f1, 4(a0); \
  flw f2, 8(a0); \
  lw  a3, 12(a0); \
  code; \
  fsflags a1, x0; \
  li a2, flags; \
  bne a0, a3, fail; \
  bne a1, a2, fail; \
  .pushsection .data; \
  .align 2; \
  test_ ## testnum ## _data: \
  .float val1; \
  .float val2; \
  .float val3; \
  .result; \
  .popsection
//Dzj:Newly added specially for rv32ud
#define TEST_FP_OP_S_INTERNAL_fcvt( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  flw f0, 0(a0); \
  flw f1, 4(a0); \
  flw f2, 8(a0); \
  lw  a3, 12(a0); \
  code; \
  fsflags a1, x0; \
  li a2, flags; \
  bne a0, a3, fail; \
  bne a1, a2, fail; \
  .pushsection .data; \
  .align 2; \
  test_ ## testnum ## _data: \
  .float val1; \
  .float val2; \
  .float val3; \
  .result; \
  .popsection

//////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////origin funct//////////////////////////////////////////////////////
#define TEST_FP_OP_D_INTERNAL( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  ld  a3, 24(a0); \
  code; \
  fsflags a1, x0; \
  li a2, flags; \
  bne a0, a3, fail; \
  bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fadd( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  lw  a4, 28(a0);\
  code; \
  bne a4, a5, fail; \
  fsflags a1, x0; \
    li a2, flags; \
  bne a6, a3, fail; \
    bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fdiv( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  lw  a4, 28(a0);\
  code; \
  bne a4, a5, fail; \
  fsflags a1, x0; \
    li a2, flags; \
  bne a6, a3, fail; \
    bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fcmp( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  code; \
  fsflags a1, x0; \
    li a2, flags; \
  bne a3, a0, fail; \
    bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fcvt( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  lw  a4, 28(a0);\
  code; \
  bne a4, a5, fail; \
  bne a6, a3, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fcvt_w( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  code; \
  fsflags a1, x0; \
  li a2, flags; \
  bne a0, a3, fail; \
  bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fmadd( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  lw  a4, 28(a0);\
  code; \
  bne a4, a5, fail; \
  fsflags a1, x0; \
  li a2, flags; \
  bne a6, a3, fail; \
  bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
//
#define TEST_FP_OP_D_INTERNAL_fmin( testnum, flags, result, val1, val2, val3, code... ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  fld f0, 0(a0); \
  fld f1, 8(a0); \
  fld f2, 16(a0); \
  lw  a3, 24(a0); \
  lw  a4, 28(a0);\
  code; \
  bne a4, a5, fail; \
  fsflags a1, x0; \
  li a2, flags; \
  bne a6, a3, fail; \
  bne a1, a2, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double val1; \
  .double val2; \
  .double val3; \
  .result; \
  .popsection
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////origin funct//////////////////////////////////////////////////////////////
#define TEST_FCVT_S_D( testnum, result, val1 ) \
  TEST_FP_OP_D_INTERNAL( testnum, 0, double result, val1, 0.0, 0.0, \
                    fcvt.s.d f3, f0; fcvt.d.s f3, f3; fmv.x.d a0, f3)


#define TEST_FCVT_S_D_fcvt( testnum, result, val1 ) \
  TEST_FP_OP_D_INTERNAL_fcvt( testnum, 0, double result, val1, 0.0, 0.0, \
                    fcvt.s.d f3, f0; fcvt.d.s f3, f3; fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
/////////////////////////////////origin funct///////////////////////////////////////////////////////////////////////
#define TEST_FCVT_D_S( testnum, result, val1 ) \
  TEST_FP_OP_S_INTERNAL( testnum, 0, float result, val1, 0.0, 0.0, \
                    fcvt.d.s f3, f0; fcvt.s.d f3, f3; fmv.x.s a0, f3)
//
#define TEST_FCVT_D_S_fcvt( testnum, result, val1 ) \
  TEST_FP_OP_S_INTERNAL_fcvt( testnum, 0, float result, val1, 0.0, 0.0, \
                    fcvt.d.s f3, f0; fcvt.s.d f3, f3; fmv.x.s a0, f3)
////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_FP_OP1_S( testnum, inst, flags, result, val1 ) \
  TEST_FP_OP_S_INTERNAL( testnum, flags, float result, val1, 0.0, 0.0, \
                    inst f3, f0; fmv.x.s a0, f3)
/////////////////////////////////origin funct////////////////////////////////////////////////
#define TEST_FP_OP1_D( testnum, inst, flags, result, val1 ) \
  TEST_FP_OP_D_INTERNAL( testnum, flags, double result, val1, 0.0, 0.0, \
                    inst f3, f0; fmv.x.d a0, f3)
//
#define TEST_FP_OP1_D_fdiv( testnum, inst, flags, result, val1 ) \
  TEST_FP_OP_D_INTERNAL_fdiv( testnum, flags, double result, val1, 0.0, 0.0, \
                    inst f3, f0;fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
/////////////////////////////////////////////////////////////////////////////////
#define TEST_FP_OP1_S_DWORD_RESULT( testnum, inst, flags, result, val1 ) \
  TEST_FP_OP_S_INTERNAL( testnum, flags, dword result, val1, 0.0, 0.0, \
                    inst f3, f0; fmv.x.s a0, f3)
///////////////////////////////origin funct/////////////////////////////////////////////////
#define TEST_FP_OP1_D_DWORD_RESULT( testnum, inst, flags, result, val1 ) \
  TEST_FP_OP_D_INTERNAL( testnum, flags, dword result, val1, 0.0, 0.0, \
                    inst f3, f0; fmv.x.d a0, f3)
//
#define TEST_FP_OP1_D_DWORD_RESULT_fdiv( testnum, inst, flags, result, val1 ) \
  TEST_FP_OP_D_INTERNAL_fdiv( testnum, flags, dword result, val1, 0.0, 0.0, \
                    inst f3, f0; fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
//
////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_FP_OP2_S( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_S_INTERNAL( testnum, flags, float result, val1, val2, 0.0, \
                    inst f3, f0, f1; fmv.x.s a0, f3)
/////////////////////////origin function/////////////////////////////////
#define TEST_FP_OP2_D( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL( testnum, flags, double result, val1, val2, 0.0, \
                    inst f3, f0, f1; fmv.x.d a0, f3)
//
#define TEST_FP_OP2_D_fadd( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL_fadd( testnum, flags, double result, val1, val2, 0.0, \
                    inst f3, f0, f1; fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
//
#define TEST_FP_OP2_D_fdiv( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL_fdiv( testnum, flags, double result, val1, val2, 0.0, \
                    inst f3, f0, f1; fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
//
#define TEST_FP_OP2_D_fmin( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL_fmin( testnum, flags, double result, val1, val2, 0.0, \
                    inst f3, f0, f1; fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
///////////////////////////////////////////////////////////////////////////////
#define TEST_FP_OP3_S( testnum, inst, flags, result, val1, val2, val3 ) \
  TEST_FP_OP_S_INTERNAL( testnum, flags, float result, val1, val2, val3, \
                    inst f3, f0, f1, f2; fmv.x.s a0, f3)
///////////////////////////origin funct//////////////////////////////////////////////////////////
#define TEST_FP_OP3_D( testnum, inst, flags, result, val1, val2, val3 ) \
  TEST_FP_OP_D_INTERNAL( testnum, flags, double result, val1, val2, val3, \
                    inst f3, f0, f1, f2; fmv.x.d a0, f3)

#define TEST_FP_OP3_D_fmadd( testnum, inst, flags, result, val1, val2, val3 ) \
  TEST_FP_OP_D_INTERNAL_fmadd( testnum, flags, double result, val1, val2, val3, \
                    inst f3, f0, f1, f2; fsd f3, 24(a0); lw a6 ,24(a0); lw a5,28(a0))
////////////////////////////////////////////////////////////////////////////////////////
#define TEST_FP_INT_OP_S( testnum, inst, flags, result, val1, rm ) \
  TEST_FP_OP_S_INTERNAL( testnum, flags, word result, val1, 0.0, 0.0, \
                    inst a0, f0, rm)
//////////////////////////origin funct//////////////////////////////////////////////////
#define TEST_FP_INT_OP_D( testnum, inst, flags, result, val1, rm ) \
  TEST_FP_OP_D_INTERNAL( testnum, flags, dword result, val1, 0.0, 0.0, \
                    inst a0, f0, rm)
//
#define TEST_FP_INT_OP_D_fcvt_w( testnum, inst, flags, result, val1, rm ) \
  TEST_FP_OP_D_INTERNAL_fcvt_w( testnum, flags, dword result, val1, 0.0, 0.0, \
                    inst a0, f0, rm)
/////////////////////////////////////////////////////////////////////////
#define TEST_FP_CMP_OP_S( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_S_INTERNAL( testnum, flags, word result, val1, val2, 0.0, \
                    inst a0, f0, f1)
///////////////////////////origin funct/////////////////////////////////////////////
#define TEST_FP_CMP_OP_D( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL( testnum, flags, dword result, val1, val2, 0.0, \
                    inst a0, f0, f1)
//
#define TEST_FP_CMP_OP_D_fcmp( testnum, inst, flags, result, val1, val2 ) \
  TEST_FP_OP_D_INTERNAL_fcmp( testnum, flags, dword result, val1, val2, 0.0, \
                    inst a0, f0, f1)
///////////////////////////////////////////////////////////////////////////////////
#define TEST_FCLASS_S(testnum, correct, input) \
  TEST_CASE(testnum, a0, correct, li a0, input; fmv.s.x fa0, a0; \
                    fclass.s a0, fa0)
/////////////////////////origin function///////////////////////////////////////////////
#define TEST_FCLASS_D(testnum, correct, input) \
  TEST_CASE(testnum, a0, correct, li a0, input; fmv.d.x fa0, a0; \
                    fclass.d a0, fa0)
//
#define TEST_FCLASS_D_fclass(testnum, correct, input) \
    TEST_CASE_fclass(testnum, a0, correct, input, fld fa0,(a0); \
                        fclass.d a0, fa0)
//////////////////////////////////////////////////////////////////////////////////////
#define TEST_INT_FP_OP_S( testnum, inst, result, val1 ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  lw  a3, 0(a0); \
  li  a0, val1; \
  inst f0, a0; \
  fsflags x0; \
  fmv.x.s a0, f0; \
  bne a0, a3, fail; \
  .pushsection .data; \
  .align 2; \
  test_ ## testnum ## _data: \
  .float result; \
  .popsection
////////////////////////////////origin function/////////////////////////////////////////////////////
#define TEST_INT_FP_OP_D( testnum, inst, result, val1 ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  ld  a3, 0(a0); \
  li  a0, val1; \
  inst f0, a0; \
  fsflags x0; \
  fmv.x.d a0, f0; \
  bne a0, a3, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double result; \
  .popsection
//Dzj:Newly added specially for rv32ud
#define TEST_INT_FP_OP_D_fcvt( testnum, inst, result, val1 ) \
test_ ## testnum: \
  li  TESTNUM, testnum; \
  la  a0, test_ ## testnum ## _data ;\
  lw  a3, 0(a0);\
  lw  a4, 4(a0);\
  li  a1, val1; \
  inst f0, a1; \
  fsflags x0; \
  fsd  f0, 0(a0);\
  lw   a6, 0(a0);\
  lw   a5, 4(a0);\
  bne a4, a5, fail; \
  bne a6, a3, fail; \
  .pushsection .data; \
  .align 3; \
  test_ ## testnum ## _data: \
  .double result; \
  .popsection
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define TEST_FSGNJS(n, insn, new_sign, rs1_sign, rs2_sign) \
  TEST_CASE(n, a0, 0x12345678 | (-(new_sign) << 31), \
    li a1, ((rs1_sign) << 31) | 0x12345678; \
    li a2, -(rs2_sign); \
    fmv.s.x f1, a1; \
    fmv.s.x f2, a2; \
    insn f0, f1, f2; \
    fmv.x.s a0, f0)
#-----------------------------------------------------------------------
# Pass and fail code (assumes test num is in TESTNUM)
#-----------------------------------------------------------------------

#define TEST_PASSFAIL \
        bne x0, TESTNUM, pass; \
fail: \
        mv a0,gp;ret
pass: \
        li a0,0; ret

 #define TESTNUM gp

	.globl float_test

float_test:

  TEST_FP_OP2_S( 2,  fadd.s, 0,                3.5,        2.5,        1.0 );
  TEST_FP_OP2_S( 3,  fadd.s, 1,              -1234,    -1235.1,        1.1 );
  TEST_FP_OP2_S( 4,  fadd.s, 1,         3.14159265, 3.14159265, 0.00000001 );

  TEST_FP_OP2_S( 5,  fsub.s, 0,                1.5,        2.5,        1.0 );
  TEST_FP_OP2_S( 6,  fsub.s, 1,              -1234,    -1235.1,       -1.1 );
  TEST_FP_OP2_S( 7,  fsub.s, 1,         3.14159265, 3.14159265, 0.00000001 );

  TEST_FP_OP2_S( 8,  fmul.s, 0,                2.5,        2.5,        1.0 );
  TEST_FP_OP2_S( 9,  fmul.s, 1,            1358.61,    -1235.1,       -1.1 );
  TEST_FP_OP2_S(10,  fmul.s, 1,      3.14159265e-8, 3.14159265, 0.00000001 );

  # Is the canonical NaN generated for Inf - Inf?
  TEST_FP_OP2_S(11,  fsub.s, 0x10, qNaNf, Inf, Inf);

  TEST_FCLASS_S(12, 1 << 0, 0xff800000 )
  TEST_FCLASS_S(13, 1 << 1, 0xbf800000 )
  TEST_FCLASS_S(14, 1 << 2, 0x807fffff )
  TEST_FCLASS_S(15, 1 << 3, 0x80000000 )
  TEST_FCLASS_S(16, 1 << 4, 0x00000000 )
  TEST_FCLASS_S(17, 1 << 5, 0x007fffff )
  TEST_FCLASS_S(18, 1 << 6, 0x3f800000 )
  TEST_FCLASS_S(19, 1 << 7, 0x7f800000 )
  TEST_FCLASS_S(20, 1 << 8, 0x7f800001 )
  TEST_FCLASS_S(21, 1 << 9, 0x7fc00000 )

  TEST_FP_CMP_OP_S(200, feq.s, 0x00, 1, -1.36, -1.36)
  TEST_FP_CMP_OP_S(201, fle.s, 0x00, 1, -1.36, -1.36)
  TEST_FP_CMP_OP_S(202, flt.s, 0x00, 0, -1.36, -1.36)

  TEST_FP_CMP_OP_S(203, feq.s, 0x00, 0, -1.37, -1.36)
  TEST_FP_CMP_OP_S(204, fle.s, 0x00, 1, -1.37, -1.36)
  TEST_FP_CMP_OP_S(205, flt.s, 0x00, 1, -1.37, -1.36)

  # Only sNaN should signal invalid for feq.
  TEST_FP_CMP_OP_S(206, feq.s, 0x00, 0, NaN, 0)
  TEST_FP_CMP_OP_S(207, feq.s, 0x00, 0, NaN, NaN)
  TEST_FP_CMP_OP_S(208, feq.s, 0x10, 0, sNaNf, 0)
  TEST_FP_CMP_OP_S(209, feq.s, 0x10, 0, 0, sNaNf)

  # qNaN should signal invalid for fle/flt.
  TEST_FP_CMP_OP_S(210, flt.s, 0x10, 0, NaN, 0)
  TEST_FP_CMP_OP_S(211, flt.s, 0x10, 0, 0,   NaN)
  TEST_FP_CMP_OP_S(212, flt.s, 0x10, 0, NaN, NaN)
  TEST_FP_CMP_OP_S(213, flt.s, 0x10, 0, sNaNf, 0)
  TEST_FP_CMP_OP_S(214, flt.s, 0x10, 0, 0, sNaNf)

  TEST_FP_CMP_OP_S(215, fle.s, 0x10, 0, NaN, 0)
  TEST_FP_CMP_OP_S(216, fle.s, 0x10, 0, 0,   NaN)
  TEST_FP_CMP_OP_S(217, fle.s, 0x10, 0, NaN, NaN)
  TEST_FP_CMP_OP_S(218, fle.s, 0x10, 0, sNaNf, 0)
  TEST_FP_CMP_OP_S(219, fle.s, 0x10, 0, 0, sNaNf)

  TEST_INT_FP_OP_S(37,  fcvt.s.w,                   2.0,  2);
  TEST_INT_FP_OP_S(38,  fcvt.s.w,                  -2.0, -2);

  TEST_INT_FP_OP_S(39, fcvt.s.wu,                   2.0,  2);
//  TEST_INT_FP_OP_S(40, fcvt.s.wu,           4.2949673e9, -2);
  TEST_INT_FP_OP_S(41, fcvt.s.wu,           2147483647, 0x7FFFFFFF);
  TEST_INT_FP_OP_S(42, fcvt.s.wu,           305419896, 0x12345678);



  TEST_FP_INT_OP_S(43,  fcvt.w.s, 0x01,         -1, -1.1, rtz);
  TEST_FP_INT_OP_S(44,  fcvt.w.s, 0x00,         -1, -1.0, rtz);
  TEST_FP_INT_OP_S(45,  fcvt.w.s, 0x01,          0, -0.9, rtz);
  TEST_FP_INT_OP_S(46,  fcvt.w.s, 0x01,          0,  0.9, rtz);
  TEST_FP_INT_OP_S(47,  fcvt.w.s, 0x00,          1,  1.0, rtz);
  TEST_FP_INT_OP_S(48,  fcvt.w.s, 0x01,          1,  1.1, rtz);
  TEST_FP_INT_OP_S(49,  fcvt.w.s, 0x10,     -1<<31, -3e9, rtz);
  TEST_FP_INT_OP_S(50,  fcvt.w.s, 0x10,  (1<<31)-1,  3e9, rtz);

  TEST_FP_INT_OP_S(51, fcvt.wu.s, 0x10,          0, -3.0, rtz);
  TEST_FP_INT_OP_S(52, fcvt.wu.s, 0x10,          0, -1.0, rtz);
  TEST_FP_INT_OP_S(53, fcvt.wu.s, 0x01,          0, -0.9, rtz);
  TEST_FP_INT_OP_S(54, fcvt.wu.s, 0x01,          0,  0.9, rtz);
  TEST_FP_INT_OP_S(55, fcvt.wu.s, 0x00,          1,  1.0, rtz);
  TEST_FP_INT_OP_S(56, fcvt.wu.s, 0x01,          1,  1.1, rtz);
  TEST_FP_INT_OP_S(57, fcvt.wu.s, 0x10,          0, -3e9, rtz);

  TEST_CASE( 58, x9, 0x000000007fffffff, la x9, tdat  ; flw f1,  0(x9); fcvt.w.s x9, f1)
  TEST_CASE( 59, x9, 0xffffffff80000000, la x9, tdat  ; flw f1,  8(x9); fcvt.w.s x9, f1)
  TEST_CASE( 60, x9, 0x000000007fffffff, la x9, tdat  ; flw f1,  4(x9); fcvt.w.s x9, f1)
  TEST_CASE( 61, x9, 0x000000007fffffff, la x9, tdat  ; flw f1, 12(x9); fcvt.w.s x9, f1)
  TEST_CASE( 62, x9,                  0, la x9, tdat  ; flw f1,  8(x9); fcvt.wu.s x9, f1)

  TEST_FP_OP2_S(63,  fdiv.s, 1, 1.1557273520668288, 3.14159265, 2.71828182 );
  TEST_FP_OP2_S(64,  fdiv.s, 1,-0.9991093838555584,      -1234,     1235.1 );
  TEST_FP_OP2_S(65,  fdiv.s, 0,         3.14159265, 3.14159265,        1.0 );
  TEST_FP_OP3_S(66,  fmadd.s, 0,                 3.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(67,  fmadd.s, 1,              1236.2, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(68,  fmadd.s, 0,               -12.0,  2.0,       -5.0,       -2.0 );
  TEST_FP_OP3_S(69, fnmadd.s, 0,                -3.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(70, fnmadd.s, 1,             -1236.2, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(71, fnmadd.s, 0,                12.0,  2.0,       -5.0,       -2.0 );
  TEST_FP_OP3_S(72,  fmsub.s, 0,                 1.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(73,  fmsub.s, 1,                1234, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(74,  fmsub.s, 0,                -8.0,  2.0,       -5.0,       -2.0 );
  TEST_FP_OP3_S(75, fnmsub.s, 0,                -1.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(76, fnmsub.s, 1,               -1234, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(77, fnmsub.s, 0,                 8.0,  2.0,       -5.0,       -2.0 );
  TEST_FP_OP2_S(78,  fmin.s, 0,        1.0,        2.5,        1.0 );
  TEST_FP_OP2_S(79,  fmin.s, 0,    -1235.1,    -1235.1,        1.1 );
  TEST_FP_OP2_S(80,  fmin.s, 0,    -1235.1,        1.1,    -1235.1 );
  TEST_FP_OP2_S(81,  fmin.s, 0,    -1235.1,        NaN,    -1235.1 );
  TEST_FP_OP2_S(82,  fmin.s, 0, 0.00000001, 3.14159265, 0.00000001 );
  TEST_FP_OP2_S(83,  fmin.s, 0,       -2.0,       -1.0,       -2.0 );
  TEST_FP_OP2_S(84,  fmax.s, 0,        2.5,        2.5,        1.0 );
  TEST_FP_OP2_S(85,  fmax.s, 0,        1.1,    -1235.1,        1.1 );
  TEST_FP_OP2_S(86,  fmax.s, 0,        1.1,        1.1,    -1235.1 );
  TEST_FP_OP2_S(87,  fmax.s, 0,    -1235.1,        NaN,    -1235.1 );
  TEST_FP_OP2_S(88,  fmax.s, 0, 3.14159265, 3.14159265, 0.00000001 );
  TEST_FP_OP2_S(89,  fmax.s, 0,       -1.0,       -1.0,       -2.0 );

  # FMIN(sNaN, x) = x
  TEST_FP_OP2_S(90,  fmax.s, 0x10, 1.0, sNaNf, 1.0);
  # FMIN(qNaN, qNaN) = canonical NaN
  TEST_FP_OP2_S(91,  fmax.s, 0x00, qNaNf, NaN, NaN);

  # -0.0 < +0.0
  TEST_FP_OP2_S(92,  fmin.s, 0,       -0.0,       -0.0,        0.0 );
  TEST_FP_OP2_S(93,  fmin.s, 0,       -0.0,        0.0,       -0.0 );
  TEST_FP_OP2_S(94,  fmax.s, 0,        0.0,       -0.0,        0.0 );
  TEST_FP_OP2_S(95,  fmax.s, 0,        0.0,        0.0,       -0.0 );


  # move FSGNJS
  TEST_CASE( 96, a1, 1, csrwi fcsr, 1; li a0, 0x1234; fssr a1, a0)
  TEST_CASE( 97, a0, 0x34, frsr a0)
  TEST_CASE( 98, a0, 0x14, frflags a0)
  TEST_CASE( 99, a0, 0x01, csrrwi a0, frm, 2)
  TEST_CASE(100, a0, 0x54, frsr a0)
  TEST_CASE(101, a0, 0x14, csrrci a0, fflags, 4)
  TEST_CASE(102, a0, 0x50, frsr a0)


  TEST_FSGNJS(103, fsgnj.s, 0, 0, 0) ; #rd = rs1 | sign(rs2)
  TEST_FSGNJS(104, fsgnj.s, 1, 0, 1)
  TEST_FSGNJS(105, fsgnj.s, 0, 1, 0)
  TEST_FSGNJS(106, fsgnj.s, 1, 1, 1)

  TEST_FSGNJS(107, fsgnjn.s, 1, 0, 0); #rd = rs1 | -sign(rs2)
  TEST_FSGNJS(108, fsgnjn.s, 0, 0, 1)
  TEST_FSGNJS(109, fsgnjn.s, 1, 1, 0)
  TEST_FSGNJS(110, fsgnjn.s, 0, 1, 1)

  TEST_FSGNJS(111, fsgnjx.s, 0, 0, 0); #rd = rs1 | (sign(rs1) ^ sign(rs2))
  TEST_FSGNJS(112, fsgnjx.s, 1, 0, 1)
  TEST_FSGNJS(113, fsgnjx.s, 1, 1, 0)
  TEST_FSGNJS(114, fsgnjx.s, 0, 1, 1)

  # recoding
  # Make sure infinities with different mantissas compare as equal.
  flw f0, minf, a0
  flw f1, three, a0
  fmul.s f1, f1, f0
  TEST_CASE(115, a0, 1, feq.s a0, f0, f1)
  TEST_CASE(116, a0, 1, fle.s a0, f0, f1)
  TEST_CASE(117, a0, 0, flt.s a0, f0, f1)

  # Likewise, but for zeroes.
  fcvt.s.w f0, x0
  li a0, 1
  fcvt.s.w f1, a0
  fmul.s f1, f1, f0
  TEST_CASE(118, a0, 1, feq.s a0, f0, f1)
  TEST_CASE(119, a0, 1, fle.s a0, f0, f1)
  TEST_CASE(120, a0, 0, flt.s a0, f0, f1)

  TEST_FP_OP2_S(300,  fdiv.s, 0   , qNaNf, 3.14159265, NaN );
  TEST_FP_OP2_S(301,  fdiv.s, 0   , qNaNf, NaN,  3.14159265);
  TEST_FP_OP2_S(302,  fdiv.s, 0x10, qNaNf, 3.14159265, sNaNf);
  TEST_FP_OP2_S(303,  fdiv.s, 0x10, qNaNf, sNaNf,  3.14159265);
  TEST_FP_OP2_S(304,  fdiv.s, 0x04, Inf, Inf,  3.14159265);
  TEST_FP_OP2_S(305,  fdiv.s, 0x04, 0,   3.14159265,  Inf);
  TEST_FP_OP2_S(306,  fdiv.s, 0x04, -Inf, -Inf,  3.14159265);
  TEST_FP_OP2_S(307,  fdiv.s, 0x04, -0, 3.14159265,  -Inf);
  TEST_FP_OP2_S(308,  fdiv.s, 0   , qNaNf, NaN, NaN );

  TEST_FP_OP2_S(320,  fadd.s, 0x10,  qNaNf,  3.14159265,  NaN);
  TEST_FP_OP2_S(321,  fadd.s, 0x10,  qNaNf,  NaN,         3.14159265);
  TEST_FP_OP2_S(322,  fadd.s, 0x10,  qNaNf,  NaN,         NaN);
  TEST_FP_OP2_S(323,  fadd.s, 0x10,  qNaNf,  sNaNf,       3.14159265);
  TEST_FP_OP2_S(324,  fadd.s, 0x10,  qNaNf,  3.14159265,  sNaNf);
  TEST_FP_OP2_S(325,  fadd.s, 0x00,  Inf,    Inf,         3.14159265);
  TEST_FP_OP2_S(326,  fadd.s, 0x00,  Inf,    3.14159265,  Inf);
  TEST_FP_OP2_S(327,  fadd.s, 0x00,  -Inf,  -Inf,         3.14159265);
  TEST_FP_OP2_S(328,  fadd.s, 0x00,  -Inf,   3.14159265, -Inf);
  TEST_FP_OP2_S(329,  fadd.s, 0x10,  qNaNf,  Inf,        -Inf);
  TEST_FP_OP2_S(330,  fadd.s, 0x00,  Inf,    Inf,         Inf);
  TEST_FP_OP2_S(331,  fadd.s, 0x00, -Inf,   -Inf,        -Inf);
  TEST_FP_OP2_S(332,  fadd.s, 0x10, nqNaNf, -Inf,         Inf);
  TEST_FP_OP2_S(333,  fadd.s, 0x11,  0f:7e38cba9,       3.14159265,  0f:7e38cba9);
  TEST_FP_OP2_S(334,  fadd.s, 0x11,  0f:7e38cba9,       0f:7e38cba9, 3.14159265);
  TEST_FP_OP2_S(335,  fadd.s, 0x10,  0f:7eb8cba9,       0f:7e38cba9, 0f:7e38cba9);

  TEST_FP_OP2_S(340,  fsub.s, 0x10,  qNaNf,  2.5,         NaN);
  TEST_FP_OP2_S(341,  fsub.s, 0x10,  qNaNf,  NaN,         NaN);
  TEST_FP_OP2_S(342,  fsub.s, 0x10,  qNaNf,  NaN,         NaN);
  TEST_FP_OP2_S(343,  fsub.s, 0x10,  qNaNf,  sNaNf,       2.5);
  TEST_FP_OP2_S(344,  fsub.s, 0x10,  qNaNf,  2.5,         sNaNf);
  TEST_FP_OP2_S(345,  fsub.s, 0x00,  Inf,    Inf,         3.14159265);
  TEST_FP_OP2_S(346,  fsub.s, 0x00, -Inf,    3.14159265,  Inf);
  TEST_FP_OP2_S(347,  fsub.s, 0x00, -Inf,   -Inf,         3.14159265);
  TEST_FP_OP2_S(348,  fsub.s, 0x00,  Inf,    3.14159265, -Inf);
  TEST_FP_OP2_S(349,  fsub.s, 0x00,  Inf,    Inf,        -Inf);
  TEST_FP_OP2_S(350,  fsub.s, 0x10,  qNaNf,  Inf,         Inf);
  TEST_FP_OP2_S(351,  fsub.s, 0x10, nqNaNf, -Inf,        -Inf);
  TEST_FP_OP2_S(352,  fsub.s, 0x00, -Inf,   -Inf,         Inf);
  TEST_FP_OP2_S(353,  fsub.s, 0x11,  0f:fe38cba9,       3.14159265,  0f:7e38cba9);
  TEST_FP_OP2_S(354,  fsub.s, 0x11,  0f:7e38cba8,       0f:7e38cba9, 3.14159265);
  TEST_FP_OP2_S(355,  fsub.s, 0x00,  0f:80000000,       0f:7e38cba9, 0f:7e38cba9);

  TEST_FP_OP2_S(360,  fmul.s, 0x10,   qNaNf, 2.5,         NaN);
  TEST_FP_OP2_S(361,  fmul.s, 0x10,   qNaNf, NaN,         NaN);
  TEST_FP_OP2_S(362,  fmul.s, 0x10,   qNaNf, NaN,         NaN);
  TEST_FP_OP2_S(363,  fmul.s, 0x10,   qNaNf, sNaNf,       2.5);
  TEST_FP_OP2_S(364,  fmul.s, 0x10,   qNaNf, 2.5,         sNaNf);
  TEST_FP_OP2_S(365,  fmul.s, 0x00,   Inf,   Inf,         3.14159265);
  TEST_FP_OP2_S(366,  fmul.s, 0x00,   Inf,   3.14159265,  Inf);
  TEST_FP_OP2_S(367,  fmul.s, 0x00,  -Inf,  -Inf,         3.14159265);
  TEST_FP_OP2_S(368,  fmul.s, 0x00,  -Inf,   3.14159265, -Inf);
  TEST_FP_OP2_S(369,  fmul.s, 0x00,  -Inf,   Inf,        -Inf);
  TEST_FP_OP2_S(370,  fmul.s, 0x00,   Inf,   Inf,         Inf);
  TEST_FP_OP2_S(371,  fmul.s, 0x00,   Inf,  -Inf,        -Inf);
  TEST_FP_OP2_S(372,  fmul.s, 0x00,  -Inf,  -Inf,         Inf);
  TEST_FP_OP2_S(373,  fmul.s, 0x10,  0f:7e38cba9,       1.0,         0f:7e38cba9);
  TEST_FP_OP2_S(374,  fmul.s, 0x10,  0f:7e38cba9,       0f:7e38cba9, 1.0        );
# TEST_FP_OP2_S(375,  fmul.s, 0x10,  0f:7f7fffff,       0f:7e38cba9, 0f:7e38cba9); //ToDo

  TEST_FP_OP3_S(380,  fmadd.s, 0x10,   qNaNf, 2.5,         2.5,        NaN      );
  TEST_FP_OP3_S(381,  fmadd.s, 0x10,   qNaNf, 2.5,         NaN,        3.1415926);
  TEST_FP_OP3_S(382,  fmadd.s, 0x10,   qNaNf, NaN,         2.5,        3.1415926);
  TEST_FP_OP3_S(383,  fmadd.s, 0x10,   qNaNf, 2.5,         NaN,        NaN      );
  TEST_FP_OP3_S(384,  fmadd.s, 0x10,   qNaNf, NaN,         NaN,        3.1415926);
  TEST_FP_OP3_S(385,  fmadd.s, 0x10,   qNaNf, NaN,         2.5,        NaN      );
  TEST_FP_OP3_S(386,  fmadd.s, 0x10,   qNaNf, NaN,         NaN,        NaN      );
  TEST_FP_OP3_S(390,  fmadd.s, 0x00,   Inf,   2.5,         2.5,        Inf      );
  TEST_FP_OP3_S(391,  fmadd.s, 0x00,   Inf,   2.5,         Inf,        3.1415926);
  TEST_FP_OP3_S(392,  fmadd.s, 0x00,   Inf,   Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(393,  fmadd.s, 0x00,   Inf,   2.5,         Inf,        Inf      );
  TEST_FP_OP3_S(394,  fmadd.s, 0x00,   Inf,   Inf,         Inf,        3.1415926);
  TEST_FP_OP3_S(395,  fmadd.s, 0x00,   Inf,   Inf,         2.5,        Inf      );
  TEST_FP_OP3_S(396,  fmadd.s, 0x00,   Inf,   Inf,         Inf,        Inf      );
  TEST_FP_OP3_S(400,  fmadd.s, 0x00,  -Inf,   2.5,         2.5,       -Inf      );
  TEST_FP_OP3_S(401,  fmadd.s, 0x00,  -Inf,   2.5,        -Inf,        3.1415926);
  TEST_FP_OP3_S(402,  fmadd.s, 0x00,  -Inf,  -Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(403,  fmadd.s, 0x00,  -Inf,   2.5,        -Inf,       -Inf      );
  TEST_FP_OP3_S(404,  fmadd.s, 0x00,   Inf,  -Inf,        -Inf,        3.1415926);
  TEST_FP_OP3_S(405,  fmadd.s, 0x00,  -Inf,  -Inf,         2.5,       -Inf      );
  TEST_FP_OP3_S(406,  fmadd.s, 0x10,  qNaNf, -Inf,        -Inf,       -Inf      );
  TEST_FP_OP3_S(407,  fmadd.s, 0x10,  qNaNf,  2.5,         Inf,       -Inf      );
  TEST_FP_OP3_S(410,  fmadd.s, 0x11,  0f:7e38cba9,       1.0,         0f:7e38cba9,   1.0        );
  TEST_FP_OP3_S(411,  fmadd.s, 0x11,  0f:7e38cba9,       0f:7e38cba9, 1.0,           1.0        );
  TEST_FP_OP3_S(412,  fmadd.s, 0x11,  0f:7e38cba9,       1.0,         1.0,           0f:7e38cba9);



  TEST_FP_OP3_S(420,  fmsub.s,0x10,   qNaNf, 2.5,         2.5,        NaN      );
  TEST_FP_OP3_S(421,  fmsub.s,0x10,   qNaNf, 2.5,         NaN,        3.1415926);
  TEST_FP_OP3_S(422,  fmsub.s,0x10,   qNaNf, NaN,         2.5,        3.1415926);
  TEST_FP_OP3_S(423,  fmsub.s,0x10,   qNaNf, 2.5,         NaN,        NaN      );
  TEST_FP_OP3_S(424,  fmsub.s,0x10,   qNaNf, NaN,         NaN,        3.1415926);
  TEST_FP_OP3_S(425,  fmsub.s,0x10,   qNaNf, NaN,         2.5,        NaN      );
  TEST_FP_OP3_S(426,  fmsub.s,0x10,   qNaNf, NaN,         NaN,        NaN      );
  TEST_FP_OP3_S(430,  fmsub.s,0x00,  -Inf,   2.5,         2.5,        Inf      );
  TEST_FP_OP3_S(431,  fmsub.s,0x00,   Inf,   2.5,         Inf,        3.1415926);
  TEST_FP_OP3_S(432,  fmsub.s,0x00,   Inf,   Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(433,  fmsub.s,0x10,   qNaNf, 2.5,         Inf,        Inf      );
  TEST_FP_OP3_S(434,  fmsub.s,0x00,   Inf,   Inf,         Inf,        3.1415926);
  TEST_FP_OP3_S(435,  fmsub.s,0x10,   qNaNf, Inf,         2.5,        Inf      );
  TEST_FP_OP3_S(436,  fmsub.s,0x10,   qNaNf, Inf,         Inf,        Inf      );
  TEST_FP_OP3_S(440,  fmsub.s,0x00,   Inf,   2.5,         2.5,       -Inf      );
  TEST_FP_OP3_S(441,  fmsub.s,0x00,  -Inf,   2.5,        -Inf,        3.1415926);
  TEST_FP_OP3_S(442,  fmsub.s,0x00,  -Inf,  -Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(443,  fmsub.s,0x10,  nqNaNf, 2.5,        -Inf,       -Inf      );
  TEST_FP_OP3_S(444,  fmsub.s,0x00,   Inf,  -Inf,        -Inf,        3.1415926);
  TEST_FP_OP3_S(445,  fmsub.s,0x10,  nqNaNf,-Inf,         2.5,       -Inf      );
  TEST_FP_OP3_S(446,  fmsub.s,0x00,   Inf,  -Inf,        -Inf,       -Inf      );
  TEST_FP_OP3_S(447,  fmsub.s,0x00,   Inf,   2.5,         Inf,       -Inf      );
  TEST_FP_OP3_S(450,  fmsub.s,0x11,  0f:7e38cba8,       1.0,         0f:7e38cba9,   1.0        );
  TEST_FP_OP3_S(451,  fmsub.s,0x11,  0f:7e38cba8,       0f:7e38cba9, 1.0,           1.0        );
  TEST_FP_OP3_S(452,  fmsub.s,0x11,  0f:fe38cba9,       1.0,         1.0,           0f:7e38cba9);

  TEST_FP_OP3_S(460,  fnmadd.s,0x10,   qNaNf, 2.5,         2.5,        NaN      );
  TEST_FP_OP3_S(461,  fnmadd.s,0x10,  nqNaNf, 2.5,         NaN,        3.1415926);
  TEST_FP_OP3_S(462,  fnmadd.s,0x10,  nqNaNf, NaN,         2.5,        3.1415926);
  TEST_FP_OP3_S(463,  fnmadd.s,0x10,   qNaNf, 2.5,         NaN,        NaN      );
  TEST_FP_OP3_S(464,  fnmadd.s,0x10,  nqNaNf, NaN,         NaN,        3.1415926);
  TEST_FP_OP3_S(465,  fnmadd.s,0x10,   qNaNf, NaN,         2.5,        NaN      );
  TEST_FP_OP3_S(466,  fnmadd.s,0x10,   qNaNf, NaN,         NaN,        NaN      );
  TEST_FP_OP3_S(470,  fnmadd.s,0x00,  -Inf,   2.5,         2.5,        Inf      );
  TEST_FP_OP3_S(471,  fnmadd.s,0x00,  -Inf,   2.5,         Inf,        3.1415926);
  TEST_FP_OP3_S(472,  fnmadd.s,0x00,  -Inf,   Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(473,  fnmadd.s,0x00,  -Inf,   2.5,         Inf,        Inf      );
  TEST_FP_OP3_S(474,  fnmadd.s,0x00,  -Inf,   Inf,         Inf,        3.1415926);
  TEST_FP_OP3_S(475,  fnmadd.s,0x00,  -Inf,   Inf,         2.5,        Inf      );
  TEST_FP_OP3_S(476,  fnmadd.s,0x00,  -Inf,   Inf,         Inf,        Inf      );
  TEST_FP_OP3_S(480,  fnmadd.s,0x00,   Inf,   2.5,         2.5,       -Inf      );
  TEST_FP_OP3_S(481,  fnmadd.s,0x00,   Inf,   2.5,        -Inf,        3.1415926);
  TEST_FP_OP3_S(482,  fnmadd.s,0x00,   Inf,  -Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(483,  fnmadd.s,0x00,   Inf,   2.5,        -Inf,       -Inf      );
  TEST_FP_OP3_S(484,  fnmadd.s,0x00,  -Inf,  -Inf,        -Inf,        3.1415926);
  TEST_FP_OP3_S(485,  fnmadd.s,0x00,   Inf,  -Inf,         2.5,       -Inf      );
  TEST_FP_OP3_S(486,  fnmadd.s,0x10,  nqNaNf,-Inf,        -Inf,       -Inf      );
  TEST_FP_OP3_S(487,  fnmadd.s,0x10,  nqNaNf, 2.5,         Inf,       -Inf      );
  TEST_FP_OP3_S(490,  fnmadd.s,0x11,  0f:fe38cbaa,       1.0,         0f:7e38cba9,   1.0        );
  TEST_FP_OP3_S(491,  fnmadd.s,0x11,  0f:fe38cbaa,       0f:7e38cba9, 1.0,           1.0        );
  TEST_FP_OP3_S(492,  fnmadd.s,0x11,  0f:fe38cbaa,       1.0,         1.0,           0f:7e38cba9);

  TEST_FP_OP3_S(500,  fnmsub.s,0x10,   qNaNf, 2.5,         2.5,        NaN      );
  TEST_FP_OP3_S(501,  fnmsub.s,0x10,  nqNaNf, 2.5,         NaN,        3.1415926);
  TEST_FP_OP3_S(502,  fnmsub.s,0x10,  nqNaNf, NaN,         2.5,        3.1415926);
  TEST_FP_OP3_S(503,  fnmsub.s,0x10,   qNaNf, 2.5,         NaN,        NaN      );
  TEST_FP_OP3_S(504,  fnmsub.s,0x10,  nqNaNf, NaN,         NaN,        3.1415926);
  TEST_FP_OP3_S(505,  fnmsub.s,0x10,   qNaNf, NaN,         2.5,        NaN      );
  TEST_FP_OP3_S(506,  fnmsub.s,0x10,   qNaNf, NaN,         NaN,        NaN      );
  TEST_FP_OP3_S(510,  fnmsub.s,0x00,   Inf,   2.5,         2.5,        Inf      );
  TEST_FP_OP3_S(511,  fnmsub.s,0x00,  -Inf,   2.5,         Inf,        3.1415926);
  TEST_FP_OP3_S(512,  fnmsub.s,0x00,  -Inf,   Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(513,  fnmsub.s,0x10,  nqNaNf, 2.5,         Inf,        Inf      );
  TEST_FP_OP3_S(514,  fnmsub.s,0x00,  -Inf,   Inf,         Inf,        3.1415926);
  TEST_FP_OP3_S(515,  fnmsub.s,0x10,  nqNaNf, Inf,         2.5,        Inf      );
  TEST_FP_OP3_S(516,  fnmsub.s,0x10,  nqNaNf, Inf,         Inf,        Inf      );
  TEST_FP_OP3_S(520,  fnmsub.s,0x00,  -Inf,   2.5,         2.5,       -Inf      );
  TEST_FP_OP3_S(521,  fnmsub.s,0x00,   Inf,   2.5,        -Inf,        3.1415926);
  TEST_FP_OP3_S(522,  fnmsub.s,0x00,   Inf,  -Inf,         2.5,        3.1415926);
  TEST_FP_OP3_S(523,  fnmsub.s,0x10,   qNaNf, 2.5,        -Inf,       -Inf      );
  TEST_FP_OP3_S(524,  fnmsub.s,0x00,  -Inf,  -Inf,        -Inf,        3.1415926);
  TEST_FP_OP3_S(525,  fnmsub.s,0x10,   qNaNf,-Inf,         2.5,       -Inf      );
  TEST_FP_OP3_S(526,  fnmsub.s,0x00,  -Inf,  -Inf,        -Inf,       -Inf      );
  TEST_FP_OP3_S(527,  fnmsub.s,0x00,  -Inf,   2.5,         Inf,       -Inf      );
  TEST_FP_OP3_S(530,  fnmsub.s,0x11,  0f:fe38cba9,        1.0,         0f:7e38cba9,   1.0        );
  TEST_FP_OP3_S(531,  fnmsub.s,0x11,  0f:fe38cba9,        0f:7e38cba9, 1.0,           1.0        );
  TEST_FP_OP3_S(532,  fnmsub.s,0x11,  0f:7e38cba8,        1.0,         1.0,           0f:7e38cba9);

  TEST_PASSFAIL

	.data
# -NaN, NaN, -inf, +inf
tdat:
.word 0xffffffff
.word 0x7fffffff
.word 0xff800000
.word 0x7f800000


minf:.float -Inf
three:.float 3.0