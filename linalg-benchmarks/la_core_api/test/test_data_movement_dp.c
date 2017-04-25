//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Authoritive Test suite for all data movement routines for the LACore
// architecture
// DOUBLE_PRECISION <-> DOUBLE_PRECISION
//==========================================================================

#include "util.h"
#include "test_util.h"

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"

//==========================================================================
// Scalar -> Scalar
//==========================================================================

TEST_CASE(copy_scalar_dp_to_scalar_adr_dp) {
  double src_data;
  DP_ARRAY(dst_data, 1);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_SCALAR_INIT(src_data, src_REG);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, 1);

    la_set_scalar_dp_reg(src_REG, src_data);
    la_set_scalar_dp_mem(dst_REG, (LaAddr)dst_data);

    la_copy(dst_REG, src_REG, 1); 

    EXPECT_ARRAY_EQUALS_VAL(dst_data, src_data, 1);
  } END_ITERATE_2_REGS()

  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(copy_scalar_adr_dp_to_scalar_adr_dp) {
  DP_ARRAY(src_data, 1);
  DP_ARRAY(dst_data, 1);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_ARRAY_INIT_VAL(src_data, src_REG, 1);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, 1);

    la_set_scalar_dp_mem(src_REG, (LaAddr)src_data);
    la_set_scalar_dp_mem(dst_REG, (LaAddr)dst_data);
    la_copy(dst_REG, src_REG, 1); 

    EXPECT_ARRAYS_EQUAL(dst_data, src_data, 1);
  } END_ITERATE_2_REGS()

  FREE(src_data);
  FREE(dst_data);
} END_TEST_CASE()


//==========================================================================
// Scalar -> Mem
//==========================================================================

TEST_CASE(copy_scalar_dp_to_mem_dp) {
  double src_data;
  DP_ARRAY(dst_data, 1);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_SCALAR_INIT(src_data, src_REG);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, 1);

    la_set_scalar_dp_reg(src_REG, src_data);
    la_set_vec_adrcnt_dp_mem(dst_REG, (LaAddr)dst_data, 1);
    la_copy(dst_REG, src_REG, 1); 

    EXPECT_ARRAY_EQUALS_VAL(dst_data, src_data, 1);
  } END_ITERATE_2_REGS()

  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(copy_scalar_adr_dp_to_mem_dp) {
  DP_ARRAY(src_data, 1);
  DP_ARRAY(dst_data, 1);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_ARRAY_INIT_VAL(dst_data, src_REG, 1);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, 1);

    la_set_scalar_dp_mem(src_REG, (LaAddr)src_data);
    la_set_vec_adrcnt_dp_mem(dst_REG, (LaAddr)dst_data, 1);
    la_copy(dst_REG, src_REG, 1); 

    EXPECT_ARRAYS_EQUAL(dst_data, src_data, 1);
  } END_ITERATE_2_REGS()

  FREE(src_data);
  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(copy_scalar_dp_multiple_to_mem_dp) {
  LaCount count = 12;
  double src_data;
  DP_ARRAY(dst_data, count+1);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_SCALAR_INIT(src_data, src_REG);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, count+1);

    la_set_scalar_dp_reg(src_REG, src_data);
    la_set_vec_adrcnt_dp_mem(dst_REG, (LaAddr)dst_data, count+1);
    la_copy(dst_REG, src_REG, count); 

    EXPECT_ARRAY_EQUALS_VAL(dst_data,       src_data, count);
    EXPECT_ARRAY_EQUALS_VAL(dst_data+count, dst_REG,  1);
  } END_ITERATE_2_REGS()

  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(copy_scalar_dp_interleaved_to_mem_dp) {
  LaCount count = 12;
  double srcA_data;
  double srcB_data;
  DP_ARRAY(dst_data, count+1);

  ITERATE_3_REGS(srcA_REG, srcB_REG, dst_REG) {
    DP_SCALAR_INIT(srcA_data, srcA_REG);
    DP_SCALAR_INIT(srcB_data, srcB_REG);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, count+1);

    la_set_scalar_dp_reg(srcA_REG, srcA_data);
    la_set_vec_dp_mem(dst_REG, (LaAddr)(dst_data+0), 2, count, 0);
    la_copy(dst_REG, srcA_REG, count/2); 

    la_set_scalar_dp_reg(srcB_REG, srcB_data);
    la_set_vec_dp_mem(dst_REG, (LaAddr)(dst_data+1), 2, count, 0);
    la_copy(dst_REG, srcB_REG, count/2); 

    EXPECT_ARRAY_STRIDE_EQUALS_VAL(dst_data,       srcA_data, count/2, 2);
    EXPECT_ARRAY_STRIDE_EQUALS_VAL(dst_data+1,     srcB_data, count/2, 2);
    EXPECT_ARRAY_EQUALS_VAL(       dst_data+count, (double)dst_REG,    1);
  } END_ITERATE_3_REGS()

  FREE(dst_data);
} END_TEST_CASE()


TEST_CASE(copy_scalar_dp_skipped_to_mem_dp) {
  LaCount count = 12;
  double src_data;
  DP_ARRAY(dst_data, count+1);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_SCALAR_INIT(src_data, src_REG);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, count+1);

    la_set_scalar_dp_reg(src_REG, src_data);
    la_set_vec_dp_mem(dst_REG, (LaAddr)dst_data, 6, 2, -11);
    la_copy(dst_REG, src_REG, 6); 

    EXPECT_ARRAY_EQUALS_VAL(dst_data+0,          src_data, 3);
    EXPECT_ARRAY_EQUALS_VAL(dst_data+3,  (double)dst_REG,  3);
    EXPECT_ARRAY_EQUALS_VAL(dst_data+6,          src_data, 3);
    EXPECT_ARRAY_EQUALS_VAL(dst_data+9,  (double)dst_REG,  3);
    EXPECT_ARRAY_EQUALS_VAL(dst_data+12, (double)dst_REG,  1);
  } END_ITERATE_2_REGS()

  FREE(dst_data);
} END_TEST_CASE()


//==========================================================================
// Mem -> Mem
//==========================================================================

TEST_CASE(copy_mem_dp_to_mem_dp) {
  LaCount count = 20;
  DP_ARRAY(src_data, count);
  DP_ARRAY(dst_data, count);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_ARRAY_INIT_VAL(src_data, count, src_REG);
    DP_ARRAY_INIT_VAL(dst_data, count, dst_REG);

    la_set_vec_adr_dp_mem(src_REG, (LaAddr)src_data);
    la_set_vec_adr_dp_mem(dst_REG, (LaAddr)dst_data);
    la_copy(dst_REG, src_REG, count); 

    EXPECT_ARRAYS_EQUAL(dst_data, src_data, count);
  } END_ITERATE_2_REGS()

  FREE(src_data);
  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(copy_mem_dp_repeat_interleave_to_mem_dp) {
  LaCount count = 20;
  DP_ARRAY(src_data, count);
  DP_ARRAY(dst_data, 2*count);

  ITERATE_2_REGS(src_REG, dst_REG) {
    DP_ARRAY_INIT_VAL(src_data, src_REG, count);
    DP_ARRAY_INIT_VAL(dst_data, dst_REG, 2*count);

    la_set_vec_dp_mem(src_REG, (LaAddr)src_data, 1, count, -count);
    la_set_vec_dp_mem(dst_REG, (LaAddr)dst_data, 2, count, -(2*count-1));
    la_copy(dst_REG, src_REG, 2*count); 

    EXPECT_ARRAY_EQUALS_VAL(dst_data, src_data[0], 2*count);
  } END_ITERATE_2_REGS()

  FREE(src_data);
  FREE(dst_data);
} END_TEST_CASE()

//==========================================================================
// Scalar -> Scratch -> Mem
//==========================================================================

TEST_CASE(copy_scalar_dp_to_scratch_dp_to_mem_dp) {
  double src_data;
  LaAddr scratch_addr;
  DP_ARRAY(dst_data, 1);

  ITERATE_3_REGS(reg_REG, sch_REG, mem_REG) {
    DP_SCALAR_INIT(src_data, reg_REG);
    DP_ARRAY_INIT_VAL(dst_data, mem_REG, 1);
    scratch_addr = (1024*sch_REG);

    la_set_scalar_dp_reg( reg_REG, src_data);
    la_set_vec_adr_dp_sch(sch_REG, scratch_addr);
    la_set_vec_adr_dp_mem(mem_REG, (LaAddr)dst_data);

    la_copy(sch_REG, reg_REG, 1); 
    la_copy(mem_REG, sch_REG, 1); 

    EXPECT_ARRAY_EQUALS_VAL(dst_data, src_data, 1);
  } END_ITERATE_3_REGS()

  FREE(dst_data);
} END_TEST_CASE()

TEST_CASE(copy_scalar_dp_multiple_to_scratch_dp_consec_to_mem_dp) {
  LaCount count = 20;
  double src_data;
  LaAddr scratch_addr;
  DP_ARRAY(dst_data, 1);

  ITERATE_3_REGS(src_reg_REG, sch_REG, dst_mem_REG) {
    DP_SCALAR_INIT(src_data, src_reg_REG);
    DP_ARRAY_INIT_VAL(dst_data, dst_mem_REG, count);
    scratch_addr = (1024*sch_REG);

    la_set_scalar_dp_reg( src_reg_REG, src_data);
    la_set_vec_dp_sch(    sch_REG,     scratch_addr, 10, 2, -19);
    la_set_vec_adr_dp_mem(dst_mem_REG, (LaAddr)dst_data);

    la_copy(sch_REG,     src_reg_REG, count);
    la_copy(dst_mem_REG, sch_REG,     count);

    EXPECT_ARRAY_EQUALS_VAL(dst_data, src_data, count);
  } END_ITERATE_3_REGS()

  FREE(dst_data);
} END_TEST_CASE()


//==========================================================================
// Mem -> Scratch/Reg -> Mem
//==========================================================================

TEST_CASE(copy_mem_dp_to_scratch_dp_to_mem_dp) {
  LaCount count = 20;
  DP_ARRAY(src_mem_data, count);
  DP_ARRAY(dst_mem_data, count);
  LaAddr sratch_addr;

  ITERATE_3_REGS(src_mem_REG, sch_REG, dst_mem_REG) {
    DP_ARRAY_INIT_INCR(src_mem_data, src_mem_REG, count);
    DP_ARRAY_INIT_VAL(dst_mem_data, dst_mem_REG, count);
    LaAddr sratch_addr = (1024*sch_REG);

    la_set_vec_adr_dp_mem(src_mem_REG, (LaAddr)src_mem_data);
    la_set_vec_adr_dp_sch(sch_REG,     sratch_addr);
    la_set_vec_adr_dp_mem(dst_mem_REG, (LaAddr)dst_mem_data);

    la_copy(sch_REG,     src_mem_REG, count); 
    la_copy(dst_mem_REG, sch_REG,     count); 

    EXPECT_ARRAYS_EQUAL(src_mem_data, dst_mem_data, count);
  } END_ITERATE_3_REGS()

  FREE(src_mem_data);
  FREE(dst_mem_data);
} END_TEST_CASE()

TEST_CASE(copy_mem_dp_to_scalar_dp_to_scratch_dp_to_mem_dp) {
  LaCount count = 20;
  DP_ARRAY(src_mem_data, 1);
  DP_ARRAY(dst_mem_data, count);
  LaAddr sratch_addr;

  ITERATE_4_REGS(src_mem_REG, reg_REG, sch_REG, dst_mem_REG) {
    DP_ARRAY_INIT_INCR(src_mem_data, src_mem_REG, 1);
    DP_ARRAY_INIT_INCR(dst_mem_data, dst_mem_REG, count);
    LaAddr sratch_addr = (1024*sch_REG);

    la_set_vec_adr_dp_mem(src_mem_REG, (LaAddr)src_mem_data);
    la_set_scalar_dp_reg( reg_REG,      0.0);
    la_set_vec_adr_dp_sch(sch_REG,      sratch_addr);
    la_set_vec_adr_dp_mem(dst_mem_REG, (LaAddr)dst_mem_data);

    la_copy(reg_REG,     src_mem_REG, 1); 
    la_copy(sch_REG,     reg_REG,     count); 
    la_copy(dst_mem_REG, sch_REG,     count); 

    EXPECT_ARRAY_EQUALS_VAL(dst_mem_data, src_mem_data[0], count);
  } END_ITERATE_4_REGS()

  FREE(src_mem_data);
  FREE(dst_mem_data);
} END_TEST_CASE()

//==========================================================================
// Test Driver
//==========================================================================

int main(){
  //scalar -> scalar
  RUN_TEST(copy_scalar_dp_to_scalar_adr_dp);
  RUN_TEST(copy_scalar_adr_dp_to_scalar_adr_dp);

  //scalar -> mem
  RUN_TEST(copy_scalar_dp_to_mem_dp);
  RUN_TEST(copy_scalar_adr_dp_to_mem_dp);
  RUN_TEST(copy_scalar_dp_multiple_to_mem_dp);
  RUN_TEST(copy_scalar_dp_interleaved_to_mem_dp);
  RUN_TEST(copy_scalar_dp_skipped_to_mem_dp);

  //mem -> mem
  RUN_TEST(copy_mem_dp_to_mem_dp);
  RUN_TEST(copy_mem_dp_repeat_interleave_to_mem_dp);

  //scalar -> scratch -> mem
  RUN_TEST(copy_scalar_dp_to_scratch_dp_to_mem_dp);
  RUN_TEST(copy_scalar_dp_multiple_to_scratch_dp_consec_to_mem_dp);

  //mem -> scratch/reg -> mem
  RUN_TEST(copy_mem_dp_to_scratch_dp_to_mem_dp);
  RUN_TEST(copy_mem_dp_to_scalar_dp_to_scratch_dp_to_mem_dp);

  return 0;
}
