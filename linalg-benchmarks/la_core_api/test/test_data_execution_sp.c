//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Authoritive Test suite for all data execution routines for the LACore
// architecture
// DOUBLE_PRECISION <-> DOUBLE_PRECISION
//==========================================================================

#include "util.h"
#include "test_util.h"

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_set_spv.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"

//==========================================================================
// single-output, single-stream
//==========================================================================

TEST_CASE(SOSS_AscaReg_add_BscaReg_mul_CscaReg_add_to_DscaMem) {
  LaCount count = 10;
  float srcA_data;
  float srcB_data;
  float srcC_data;
  SP_ARRAY(dst_data, 1);
  float expected_data;

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_SCALAR_INIT(srcA_data, srcA_REG);
    SP_SCALAR_INIT(srcB_data, srcB_REG);
    SP_SCALAR_INIT(srcC_data, (srcC_REG+1));
    SP_ARRAY_INIT_VAL(dst_data, dst_REG, 1);

    //set the expected values
    expected_data = (float)(count * (srcA_REG+srcB_REG) * (srcC_REG+1));

    //now perform data op
    la_set_scalar_sp_reg(srcA_REG, srcA_data);
    la_set_scalar_sp_reg(srcB_REG, srcB_data);
    la_set_scalar_sp_reg(srcC_REG, srcC_data);
    la_set_scalar_sp_mem(dst_REG,  (LaAddr)dst_data);

    la_AaddBmulC_sum(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAY_EQUALS_VAL(dst_data, expected_data, 1);
  } END_ITERATE_4_REGS()

  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(SOSS_AscaReg_sub_BscaMem_div_CscaSch_add_to_DscaMem) {
  LaCount count = 10;
  float srcA_data;
  SP_ARRAY(srcB_data, 1);
  SP_ARRAY(srcC_data, 1);
  LaAddr srcC_scratch_addr;
  SP_ARRAY(dst_data, 1);
  float expected_data;

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_SCALAR_INIT(   srcA_data, srcA_REG);
    SP_ARRAY_INIT_VAL(srcB_data, srcB_REG,     1);
    SP_ARRAY_INIT_VAL(srcC_data, (srcC_REG+1), 1);
    SP_ARRAY_INIT_VAL(dst_data,  dst_REG,      1);
    srcC_scratch_addr = (1024*srcC_REG);

    //set the expected values
    expected_data = ((float)count* ((float)srcA_REG - (float)srcB_REG)) /
                      ((float)srcC_REG+1.0);

    //move srcC_data to scratchpad
    la_set_scalar_sp_mem(srcC_REG, (LaAddr)srcC_data);
    la_set_scalar_sp_sch(srcA_REG, srcC_scratch_addr);
    la_copy(srcA_REG, srcC_REG, 1);

    //now perform data op
    la_set_scalar_sp_reg(srcA_REG, srcA_data);
    la_set_scalar_sp_mem(srcB_REG, (LaAddr)srcB_data);
    la_set_scalar_sp_sch(srcC_REG, srcC_scratch_addr);
    la_set_scalar_sp_mem(dst_REG,  (LaAddr)dst_data);

    la_AsubBdivC_sum(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAY_EQUALS_VAL(dst_data, expected_data, 1);
  } END_ITERATE_4_REGS()

  FREE(srcB_data);
  FREE(srcC_data);
  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(SOSS_AvecMem_add_BvecSch_mul_CscaMem_add_to_DvecMem) {
  LaCount count = 10;
  SP_ARRAY(srcA_data, count);
  SP_ARRAY(srcB_data, count);
  LaAddr srcB_scratch_addr;
  SP_ARRAY(srcC_data, 1);
  SP_ARRAY(dst_data, 1);
  float expected_data;

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_ARRAY_INIT_INCR(srcA_data, srcA_REG,     count);
    SP_ARRAY_INIT_INCR(srcB_data, srcB_REG,     count);
    SP_ARRAY_INIT_VAL(srcC_data,  (srcC_REG+1), 1);
    SP_ARRAY_INIT_VAL(dst_data,   dst_REG,      1);
    srcB_scratch_addr = (1024*srcB_REG);

    //set the expected value
    expected_data = 0.0;
    for(LaCount i=0; i<count; ++i){
      expected_data += ((srcA_data[i] + srcB_data[i]) * srcC_data[0]);
    }

    //move srcB_data to scratchpad
    la_set_vec_adr_sp_mem(srcB_REG, (LaAddr)srcB_data);
    la_set_vec_adr_sp_sch(srcA_REG, srcB_scratch_addr);
    la_copy(srcA_REG, srcB_REG, count);

    //now perform data op
    la_set_vec_adr_sp_mem(srcA_REG, (LaAddr)srcA_data);
    la_set_vec_adr_sp_sch(srcB_REG, srcB_scratch_addr);
    la_set_scalar_sp_mem(srcC_REG, (LaAddr)srcC_data);
    la_set_scalar_sp_mem(dst_REG,  (LaAddr)dst_data);

    la_AaddBmulC_sum(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAY_EQUALS_VAL(dst_data, expected_data, 1);
  } END_ITERATE_4_REGS()

  FREE(srcA_data);
  FREE(srcB_data);
  FREE(srcC_data);
  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(SOSS_AvecMem_add_BvecMem_mul_CscaMem_add_to_DvecSch) {
  LaCount count = 10;
  SP_ARRAY(srcA_data, count);
  SP_ARRAY(srcB_data, count);
  SP_ARRAY(srcC_data, 1);
  SP_ARRAY(dst_data, 1);
  LaAddr dst_scratch_addr;
  float expected_data;

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_ARRAY_INIT_INCR(srcA_data, srcA_REG,     count);
    SP_ARRAY_INIT_INCR(srcB_data, srcB_REG,     count);
    SP_ARRAY_INIT_VAL(srcC_data,  (srcC_REG+1), 1);
    SP_ARRAY_INIT_VAL(dst_data,   dst_REG,      1);
    dst_scratch_addr = (1024*dst_REG);

    //set the expected value
    expected_data = 0.0;
    for(LaCount i=0; i<count; ++i){
      expected_data += ((srcA_data[i] + srcB_data[i]) * srcC_data[0]);
    }

    //perform data op
    la_set_vec_adr_sp_mem(srcA_REG, (LaAddr)srcA_data);
    la_set_vec_adr_sp_mem(srcB_REG, (LaAddr)srcB_data);
    la_set_scalar_sp_mem(srcC_REG, (LaAddr)srcC_data);
    la_set_scalar_sp_sch(dst_REG,  dst_scratch_addr);

    la_AaddBmulC_sum(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    //move dst_data to memory
    la_set_scalar_sp_sch(srcA_REG, dst_scratch_addr);
    la_set_scalar_sp_mem(dst_REG, (LaAddr)dst_data);
    la_copy(dst_REG, srcA_REG, 1);

    EXPECT_ARRAY_EQUALS_VAL(dst_data, expected_data, 1);
  } END_ITERATE_4_REGS()

  FREE(srcA_data);
  FREE(srcB_data);
  FREE(srcC_data);
  FREE(dst_data);
} END_TEST_CASE()

//==========================================================================
// single-output, multi-stream
//==========================================================================

TEST_CASE(SOMS_AscaReg_add_BvecMem_div_CscaReg_add_to_DvecMem) {
  LaCount count = 20;
  LaCount multi_count = 4;
  float srcA_data;
  SP_ARRAY(srcB_data, count);
  float srcC_data;
  SP_ARRAY(dst_data, multi_count);
  SP_ARRAY(expected_data, multi_count);

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_SCALAR_INIT(    srcA_data, srcA_REG);
    SP_ARRAY_INIT_INCR(srcB_data, srcB_REG, count);
    SP_SCALAR_INIT(    srcC_data, (srcC_REG+1));
    SP_ARRAY_INIT_INCR(dst_data,  dst_REG,  multi_count);

    //set the expected values
    for(LaCount i=0; i<multi_count; ++i){
      expected_data[i] = 0;
      for(LaCount j=0; j<5; ++j){
        expected_data[i] += ((srcA_data + srcB_data[5*i+j]) / srcC_data);
      }
    }

    //perform data op
    la_set_scalar_sp_reg(    srcA_REG, srcA_data);
    la_set_vec_adrcnt_sp_mem(srcB_REG, (LaAddr)srcB_data, count/multi_count);
    la_set_scalar_sp_reg(    srcC_REG, srcC_data);
    la_set_vec_adrcnt_sp_mem(dst_REG,  (LaAddr)dst_data,  multi_count);

    la_AaddBdivC_sum_multi(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAYS_EQUAL(dst_data, expected_data, multi_count);
  } END_ITERATE_4_REGS()

  FREE(srcB_data);
  FREE(dst_data);
  FREE(expected_data);
} END_TEST_CASE()

TEST_CASE(SOMS_AvecSch_add_BvecMem_div_CscaMem_add_to_DvecMem) {
  LaCount count = 20;
  LaCount multi_count = 4;
  LaCount src_count = count/multi_count;

  SP_ARRAY(srcA_data, src_count);
  LaAddr srcA_scratch_addr;
  SP_ARRAY(srcB_data, count);
  SP_ARRAY(srcC_data, 1);
  SP_ARRAY(dst_data, multi_count);
  SP_ARRAY(expected_data, multi_count);

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_ARRAY_INIT_INCR(srcA_data, srcA_REG,     src_count);
    SP_ARRAY_INIT_INCR(srcB_data, srcB_REG,     count);
    SP_ARRAY_INIT_INCR(srcC_data, (srcC_REG+1), 1);
    SP_ARRAY_INIT_INCR(dst_data,  dst_REG,      multi_count);
    srcA_scratch_addr = (1024*srcA_REG);

    //set the expected values
    for(LaCount i=0; i<multi_count; ++i){
      expected_data[i] = 0;
      for(LaCount j=0; j<src_count; ++j){
        expected_data[i] += ((srcA_data[j] + srcB_data[src_count*i+j]) / 
                              srcC_data[0]);
      }
    }

    //move srcA_data to scratchpad
    la_set_vec_adr_sp_mem(srcA_REG, (LaAddr)srcA_data);
    la_set_vec_adr_sp_sch(srcB_REG, srcA_scratch_addr);
    la_copy(srcB_REG, srcA_REG, src_count);

    //perform data op
    la_set_vec_sp_sch(srcA_REG, srcA_scratch_addr, 1, src_count, -src_count);
    la_set_vec_adrcnt_sp_mem(srcB_REG, (LaAddr)srcB_data, src_count);
    la_set_scalar_sp_mem(    srcC_REG, (LaAddr)srcC_data);
    la_set_vec_adrcnt_sp_mem(dst_REG,  (LaAddr)dst_data,  multi_count);

    la_AaddBdivC_sum_multi(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAYS_EQUAL(dst_data, expected_data, multi_count);
  } END_ITERATE_4_REGS()

  FREE(srcA_data);
  FREE(srcB_data);
  FREE(srcC_data);
  FREE(dst_data);
  FREE(expected_data);
} END_TEST_CASE()

//NOTE: got super lazy here
//A is sparse 4x4 with {1,2,3,4} along diagonal
//B is vector {2,2,2,2}
//C is scalar {5}
//D should be {7,9,11,13}
TEST_CASE(SOMS_AspvMem_mul_BvecMem_add_CscaReg_add_to_DvecMem) {
  float srcA_data[4];
  uint32_t srcA_idx[4];
  uint32_t srcA_jdx[4];
  float srcB_data[4];
  float srcC_data = 5.0;
  float dst_data[4];
  float expected_data[4];

  //setup sparse ptrs and data
  for(LaAddr i=0; i<4; ++i) {
    srcA_data[i] = (float) (i+1);
    srcA_jdx[i] = i;
    srcA_idx[i] = (i+1);

    srcB_data[i] = 2.0;
    expected_data[i] = 4*5.0 + 2*((float)(i+1));
  }
  LaAddr srcA_dptr = (LaAddr) srcA_data;
  LaAddr srcA_iptr = (LaAddr) srcA_idx;
  LaAddr srcA_jptr = (LaAddr) srcA_jdx;

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    for(LaAddr i=0; i<4; ++i){
      dst_data[i] = 100.0; //set to some random value
    }

    //perform data op
    la_set_spv_nrm_sp_mem(srcA_REG, srcA_dptr, srcA_iptr, srcA_jptr, 4, 4, 0);
    la_set_vec_sp_mem(srcB_REG, (LaAddr)srcB_data, 1, 4, -4);
    la_set_scalar_sp_reg(srcC_REG, srcC_data);
    la_set_vec_sp_mem(dst_REG,  (LaAddr)dst_data,  1, 4, 0);

    la_AmulBaddC_sum_multi(dst_REG, srcA_REG, srcB_REG, srcC_REG, 16);

    EXPECT_ARRAYS_EQUAL(dst_data, expected_data, 4);
  } END_ITERATE_4_REGS()

} END_TEST_CASE()

//==========================================================================
// vector-output, single-stream
//==========================================================================

TEST_CASE(VOSS_AscaReg_add_BscaReg_mul_CscaReg_to_DvecMem) {
  LaCount count = 10;
  float srcA_data;
  float srcB_data;
  float srcC_data;
  SP_ARRAY(dst_data, count);
  float expected_data;

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_SCALAR_INIT(srcA_data, srcA_REG);
    SP_SCALAR_INIT(srcB_data, srcB_REG);
    SP_SCALAR_INIT(srcC_data, (srcC_REG+1));
    SP_ARRAY_INIT_VAL(dst_data, dst_REG, count);

    //set the expected values
    expected_data = (srcA_data + srcB_data) * srcC_data;

    //perform data op
    la_set_scalar_sp_reg(srcA_REG, srcA_data);
    la_set_scalar_sp_reg(srcB_REG, srcB_data);
    la_set_scalar_sp_reg(srcC_REG, srcC_data);
    la_set_vec_adr_sp_mem(dst_REG, (LaAddr)dst_data);

    la_AaddBmulC(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAY_EQUALS_VAL(dst_data, expected_data, count);
  } END_ITERATE_4_REGS()

  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(VOSS_AvecMem_sub_BscaReg_div_CvecSch_to_DvecMem) {
  LaCount count = 10;
  SP_ARRAY(srcA_data, count);
  float srcB_data;
  SP_ARRAY(srcC_data, count);
  LaAddr srcC_scratch_addr;
  SP_ARRAY(dst_data, count);
  SP_ARRAY(expected_data, count);

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_ARRAY_INIT_VAL(srcA_data, srcA_REG, count);
    SP_SCALAR_INIT(srcB_data, srcB_REG);
    SP_ARRAY_INIT_VAL(srcC_data, (srcC_REG+1), count);
    SP_ARRAY_INIT_VAL(dst_data, dst_REG, count);

    //set the expected values
    for(LaCount i=0; i<count; ++i){
      expected_data[i] = (srcA_data[i] + srcB_data) / srcC_data[i];
    }

    //move srcC_data to scratchpad
    la_set_vec_adr_sp_mem(srcA_REG, (LaAddr)srcC_data);
    la_set_vec_adr_sp_sch(srcB_REG, srcC_scratch_addr);
    la_copy(srcB_REG, srcA_REG, count);

    //perform data op
    la_set_vec_adr_sp_mem(srcA_REG, (LaAddr)srcA_data);
    la_set_scalar_sp_reg(srcB_REG,  srcB_data);
    la_set_vec_adr_sp_sch(srcC_REG, srcC_scratch_addr);
    la_set_vec_adr_sp_mem(dst_REG,  (LaAddr)dst_data);

    la_AaddBdivC(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    EXPECT_ARRAYS_EQUAL(dst_data, expected_data, count);
  } END_ITERATE_4_REGS()

  FREE(srcA_data);
  FREE(srcC_data);
  FREE(dst_data);
  FREE(expected_data);
} END_TEST_CASE()

TEST_CASE(VOSS_AvecMem_sub_BvecSch_mul_CscaReg_to_DvecSch) {
  LaCount count = 10;
  SP_ARRAY(srcA_data, count);
  SP_ARRAY(srcB_data, count);
  LaAddr srcB_scratch_addr;
  float srcC_data;
  SP_ARRAY(dst_data, count);
  LaAddr dst_scratch_addr;
  SP_ARRAY(expected_data, count);

  ITERATE_4_REGS(srcA_REG, srcB_REG, srcC_REG, dst_REG) {
    SP_ARRAY_INIT_VAL(srcA_data, srcA_REG,    count);
    SP_ARRAY_INIT_VAL(srcB_data, srcB_REG,    count);
    SP_SCALAR_INIT(   srcC_data, (srcC_REG+1));
    SP_ARRAY_INIT_VAL(dst_data,  dst_REG,     count);
    srcB_scratch_addr = (1024*srcB_REG);
    dst_scratch_addr = (1024*dst_REG);    

    //set the expected values
    for(LaCount i=0; i<count; ++i){
      expected_data[i] = (srcA_data[i] - srcB_data[i]) * srcC_data;
    }

    //move srcB_data to scratchpad
    la_set_vec_adr_sp_mem(srcA_REG, (LaAddr)srcB_data);
    la_set_vec_adr_sp_sch(srcB_REG, srcB_scratch_addr);
    la_copy(srcB_REG, srcA_REG, count);

    //perform data op
    la_set_vec_adr_sp_mem(srcA_REG,  (LaAddr)srcA_data);
    la_set_vec_adr_sp_sch(srcB_REG,  srcB_scratch_addr);
    la_set_scalar_sp_reg( srcC_REG,  srcC_data);
    la_set_vec_adr_sp_sch(dst_REG,   dst_scratch_addr);

    la_AsubBmulC(dst_REG, srcA_REG, srcB_REG, srcC_REG, count);

    //move srcD_data out of scratchpad
    la_set_vec_adr_sp_sch(srcA_REG, dst_scratch_addr);
    la_set_vec_adr_sp_mem(srcB_REG, (LaAddr)dst_data);
    la_copy(srcB_REG, srcA_REG, count);

    EXPECT_ARRAYS_EQUAL(dst_data, expected_data, count);
  } END_ITERATE_4_REGS()

  FREE(srcA_data);
  FREE(srcB_data);
  FREE(dst_data);
  FREE(expected_data);
} END_TEST_CASE()


//==========================================================================
// Test Driver
//==========================================================================

//TODO: test reduce > or < operations (NOT YET TESTED)
int main(){
  //single-output, single-stream
  RUN_TEST(SOSS_AscaReg_add_BscaReg_mul_CscaReg_add_to_DscaMem);
  RUN_TEST(SOSS_AscaReg_sub_BscaMem_div_CscaSch_add_to_DscaMem);
  RUN_TEST(SOSS_AvecMem_add_BvecSch_mul_CscaMem_add_to_DvecMem);
  RUN_TEST(SOSS_AvecMem_add_BvecMem_mul_CscaMem_add_to_DvecSch);   

  //single-output, multi-stream
  RUN_TEST(SOMS_AscaReg_add_BvecMem_div_CscaReg_add_to_DvecMem);
  RUN_TEST(SOMS_AvecSch_add_BvecMem_div_CscaMem_add_to_DvecMem);
  RUN_TEST(SOMS_AspvMem_mul_BvecMem_add_CscaReg_add_to_DvecMem);

  //vector-output, single-stream
  RUN_TEST(VOSS_AscaReg_add_BscaReg_mul_CscaReg_to_DvecMem);
  RUN_TEST(VOSS_AvecMem_sub_BscaReg_div_CvecSch_to_DvecMem);
  RUN_TEST(VOSS_AvecMem_sub_BvecSch_mul_CscaReg_to_DvecSch);

  return 0;
}
