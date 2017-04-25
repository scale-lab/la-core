//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Test File to make sure that the LaCore API fully x-compiles on riscv-gcc.
// This test should be run before any other tests (such as the spike test)
//==========================================================================

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_set_spv.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"


void check_set_scalar(){
  double dp   = 3.0;
  float  sp   = 2.0;
  LaAddr addr1 = 100;
  LaAddr addr2 = 200;

  //put scalar in lareg
  la_set_scalar_dp_reg(0, dp);
  la_set_scalar_dp_reg(1, dp);
  la_set_scalar_dp_reg(2, dp);
  la_set_scalar_dp_reg(3, dp);
  la_set_scalar_dp_reg(4, dp);
  la_set_scalar_dp_reg(5, dp);
  la_set_scalar_dp_reg(6, dp);
  la_set_scalar_dp_reg(7, dp);

  la_set_scalar_sp_reg(0, sp);
  la_set_scalar_sp_reg(1, sp);
  la_set_scalar_sp_reg(2, sp);
  la_set_scalar_sp_reg(3, sp);
  la_set_scalar_sp_reg(4, sp);
  la_set_scalar_sp_reg(5, sp);
  la_set_scalar_sp_reg(6, sp);
  la_set_scalar_sp_reg(7, sp);

  //scalar in main memory
  la_set_scalar_dp_mem(0, addr1);
  la_set_scalar_dp_mem(1, addr1);
  la_set_scalar_dp_mem(2, addr1);
  la_set_scalar_dp_mem(3, addr1);
  la_set_scalar_dp_mem(4, addr1);
  la_set_scalar_dp_mem(5, addr1);
  la_set_scalar_dp_mem(6, addr1);
  la_set_scalar_dp_mem(7, addr1);

  la_set_scalar_sp_mem(0, addr1);
  la_set_scalar_sp_mem(1, addr1);
  la_set_scalar_sp_mem(2, addr1);
  la_set_scalar_sp_mem(3, addr1);
  la_set_scalar_sp_mem(4, addr1);
  la_set_scalar_sp_mem(5, addr1);
  la_set_scalar_sp_mem(6, addr1);
  la_set_scalar_sp_mem(7, addr1);

  //scalar in scratchpad
  la_set_scalar_dp_sch(0, addr2);
  la_set_scalar_dp_sch(1, addr2);
  la_set_scalar_dp_sch(2, addr2);
  la_set_scalar_dp_sch(3, addr2);
  la_set_scalar_dp_sch(4, addr2);
  la_set_scalar_dp_sch(5, addr2);
  la_set_scalar_dp_sch(6, addr2);
  la_set_scalar_dp_sch(7, addr2);

  la_set_scalar_sp_sch(0, addr2);
  la_set_scalar_sp_sch(1, addr2);
  la_set_scalar_sp_sch(2, addr2);
  la_set_scalar_sp_sch(3, addr2);
  la_set_scalar_sp_sch(4, addr2);
  la_set_scalar_sp_sch(5, addr2);
  la_set_scalar_sp_sch(6, addr2);
  la_set_scalar_sp_sch(7, addr2);
}

void check_set_vec(){
  //set vec-addr (default stride, skip, count)
  LaAddr      addr1   = 100;

  la_set_vec_adr_dp_mem(0, addr1);
  la_set_vec_adr_dp_mem(1, addr1);
  la_set_vec_adr_dp_mem(2, addr1);
  la_set_vec_adr_dp_mem(3, addr1);
  la_set_vec_adr_dp_mem(4, addr1);
  la_set_vec_adr_dp_mem(5, addr1);
  la_set_vec_adr_dp_mem(6, addr1);
  la_set_vec_adr_dp_mem(7, addr1);

  la_set_vec_adr_dp_sch(0, addr1);
  la_set_vec_adr_dp_sch(1, addr1);
  la_set_vec_adr_dp_sch(2, addr1);
  la_set_vec_adr_dp_sch(3, addr1);
  la_set_vec_adr_dp_sch(4, addr1);
  la_set_vec_adr_dp_sch(5, addr1);
  la_set_vec_adr_dp_sch(6, addr1);
  la_set_vec_adr_dp_sch(7, addr1);

  la_set_vec_adr_sp_mem(0, addr1);
  la_set_vec_adr_sp_mem(1, addr1);
  la_set_vec_adr_sp_mem(2, addr1);
  la_set_vec_adr_sp_mem(3, addr1);
  la_set_vec_adr_sp_mem(4, addr1);
  la_set_vec_adr_sp_mem(5, addr1);
  la_set_vec_adr_sp_mem(6, addr1);
  la_set_vec_adr_sp_mem(7, addr1);

  la_set_vec_adr_sp_sch(0, addr1);
  la_set_vec_adr_sp_sch(1, addr1);
  la_set_vec_adr_sp_sch(2, addr1);
  la_set_vec_adr_sp_sch(3, addr1);
  la_set_vec_adr_sp_sch(4, addr1);
  la_set_vec_adr_sp_sch(5, addr1);
  la_set_vec_adr_sp_sch(6, addr1);
  la_set_vec_adr_sp_sch(7, addr1);

  //set vec-addr (skip, count)
  LaAddr      addr2   = 200;
  LaVecCount  count2  = 64;

  la_set_vec_adrcnt_dp_mem(0, addr2, count2);
  la_set_vec_adrcnt_dp_mem(1, addr2, count2);
  la_set_vec_adrcnt_dp_mem(2, addr2, count2);
  la_set_vec_adrcnt_dp_mem(3, addr2, count2);
  la_set_vec_adrcnt_dp_mem(4, addr2, count2);
  la_set_vec_adrcnt_dp_mem(5, addr2, count2);
  la_set_vec_adrcnt_dp_mem(6, addr2, count2);
  la_set_vec_adrcnt_dp_mem(7, addr2, count2);

  la_set_vec_adrcnt_dp_sch(0, addr2, count2);
  la_set_vec_adrcnt_dp_sch(1, addr2, count2);
  la_set_vec_adrcnt_dp_sch(2, addr2, count2);
  la_set_vec_adrcnt_dp_sch(3, addr2, count2);
  la_set_vec_adrcnt_dp_sch(4, addr2, count2);
  la_set_vec_adrcnt_dp_sch(5, addr2, count2);
  la_set_vec_adrcnt_dp_sch(6, addr2, count2);
  la_set_vec_adrcnt_dp_sch(7, addr2, count2);

  la_set_vec_adrcnt_sp_mem(0, addr2, count2);
  la_set_vec_adrcnt_sp_mem(1, addr2, count2);
  la_set_vec_adrcnt_sp_mem(2, addr2, count2);
  la_set_vec_adrcnt_sp_mem(3, addr2, count2);
  la_set_vec_adrcnt_sp_mem(4, addr2, count2);
  la_set_vec_adrcnt_sp_mem(5, addr2, count2);
  la_set_vec_adrcnt_sp_mem(6, addr2, count2);
  la_set_vec_adrcnt_sp_mem(7, addr2, count2);

  la_set_vec_adrcnt_sp_sch(0, addr2, count2);
  la_set_vec_adrcnt_sp_sch(1, addr2, count2);
  la_set_vec_adrcnt_sp_sch(2, addr2, count2);
  la_set_vec_adrcnt_sp_sch(3, addr2, count2);
  la_set_vec_adrcnt_sp_sch(4, addr2, count2);
  la_set_vec_adrcnt_sp_sch(5, addr2, count2);
  la_set_vec_adrcnt_sp_sch(6, addr2, count2);
  la_set_vec_adrcnt_sp_sch(7, addr2, count2);


  //set vec-addr (all fields)
  LaAddr      addr3   = 300;
  LaVecCount  count3  = 128;
  LaVecStride stride3 = 2;
  LaVecSkip   skip3   = -128;

  la_set_vec_dp_mem(0, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(1, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(2, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(3, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(4, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(5, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(6, addr3, stride3, count3, skip3);
  la_set_vec_dp_mem(7, addr3, stride3, count3, skip3);

  la_set_vec_dp_sch(0, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(1, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(2, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(3, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(4, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(5, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(6, addr3, stride3, count3, skip3);
  la_set_vec_dp_sch(7, addr3, stride3, count3, skip3);

  la_set_vec_sp_mem(0, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(1, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(2, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(3, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(4, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(5, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(6, addr3, stride3, count3, skip3);
  la_set_vec_sp_mem(7, addr3, stride3, count3, skip3);

  la_set_vec_sp_sch(0, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(1, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(2, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(3, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(4, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(5, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(6, addr3, stride3, count3, skip3);
  la_set_vec_sp_sch(7, addr3, stride3, count3, skip3);
}


void check_set_spv(){

  LaAddr      data_ptr  = 100;
  LaAddr      idx_ptr   = 200;
  LaAddr      jdx_ptr   = 300;
  LaSpvCount  idx_cnt   = 400;
  LaSpvCount  jdx_cnt   = 500;
  LaSpvCount  dskip     = 600;

  la_set_spv_nrm_dp_mem(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_mem(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_tns_dp_mem(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_mem(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_nrm_sp_mem(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_mem(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_tns_sp_mem(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_mem(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_nrm_dp_sch(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_dp_sch(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_tns_dp_sch(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_dp_sch(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_nrm_sp_sch(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_nrm_sp_sch(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);

  la_set_spv_tns_sp_sch(0, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(1, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(2, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(3, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(4, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(5, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(6, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
  la_set_spv_tns_sp_sch(7, data_ptr, idx_ptr, jdx_ptr, idx_cnt, jdx_cnt, dskip);
}

void check_copy(){

  //first half of copy ops
  LaCount count1 = 2048;

  la_copy(0, 0, count1);
  la_copy(0, 1, count1);
  la_copy(0, 2, count1);
  la_copy(0, 3, count1);
  la_copy(0, 4, count1);
  la_copy(0, 5, count1);
  la_copy(0, 6, count1);
  la_copy(0, 7, count1);
  la_copy(1, 0, count1);
  la_copy(1, 1, count1);
  la_copy(1, 2, count1);
  la_copy(1, 3, count1);
  la_copy(1, 4, count1);
  la_copy(1, 5, count1);
  la_copy(1, 6, count1);
  la_copy(1, 7, count1);
  la_copy(2, 0, count1);
  la_copy(2, 1, count1);
  la_copy(2, 2, count1);
  la_copy(2, 3, count1);
  la_copy(2, 4, count1);
  la_copy(2, 5, count1);
  la_copy(2, 6, count1);
  la_copy(2, 7, count1);
  la_copy(3, 0, count1);
  la_copy(3, 1, count1);
  la_copy(3, 2, count1);
  la_copy(3, 3, count1);
  la_copy(3, 4, count1);
  la_copy(3, 5, count1);
  la_copy(3, 6, count1);
  la_copy(3, 7, count1);

  //second half of copy ops
  LaCount count2 = 4096;

  la_copy(4, 0, count2);
  la_copy(4, 1, count2);
  la_copy(4, 2, count2);
  la_copy(4, 3, count2);
  la_copy(4, 4, count2);
  la_copy(4, 5, count2);
  la_copy(4, 6, count2);
  la_copy(4, 7, count2);
  la_copy(5, 0, count2);
  la_copy(5, 1, count2);
  la_copy(5, 2, count2);
  la_copy(5, 3, count2);
  la_copy(5, 4, count2);
  la_copy(5, 5, count2);
  la_copy(5, 6, count2);
  la_copy(5, 7, count2);
  la_copy(6, 0, count2);
  la_copy(6, 1, count2);
  la_copy(6, 2, count2);
  la_copy(6, 3, count2);
  la_copy(6, 4, count2);
  la_copy(6, 5, count2);
  la_copy(6, 6, count2);
  la_copy(6, 7, count2);
  la_copy(7, 0, count2);
  la_copy(7, 1, count2);
  la_copy(7, 2, count2);
  la_copy(7, 3, count2);
  la_copy(7, 4, count2);
  la_copy(7, 5, count2);
  la_copy(7, 6, count2);
  la_copy(7, 7, count2);
}

void check_data_op(){

  //test single-out single-stream
  LaCount count1 = 1000;

  la_AaddBmulC_min(0, 1, 2, 3, count1);
  la_AmulBaddC_min(0, 1, 2, 3, count1);
  la_AsubBmulC_min(0, 1, 2, 3, count1);
  la_AmulBsubC_min(0, 1, 2, 3, count1);
  la_AaddBdivC_min(0, 1, 2, 3, count1);
  la_AdivBaddC_min(0, 1, 2, 3, count1);
  la_AsubBdivC_min(0, 1, 2, 3, count1);
  la_AdivBsubC_min(0, 1, 2, 3, count1);

  la_AaddBmulC_max(0, 1, 2, 3, count1);
  la_AmulBaddC_max(0, 1, 2, 3, count1);
  la_AsubBmulC_max(0, 1, 2, 3, count1);
  la_AmulBsubC_max(0, 1, 2, 3, count1);
  la_AaddBdivC_max(0, 1, 2, 3, count1);
  la_AdivBaddC_max(0, 1, 2, 3, count1);
  la_AsubBdivC_max(0, 1, 2, 3, count1);
  la_AdivBsubC_max(0, 1, 2, 3, count1);

  la_AaddBmulC_sum(0, 1, 2, 3, count1);
  la_AmulBaddC_sum(0, 1, 2, 3, count1);
  la_AsubBmulC_sum(0, 1, 2, 3, count1);
  la_AmulBsubC_sum(0, 1, 2, 3, count1);
  la_AaddBdivC_sum(0, 1, 2, 3, count1);
  la_AdivBaddC_sum(0, 1, 2, 3, count1);
  la_AsubBdivC_sum(0, 1, 2, 3, count1);
  la_AdivBsubC_sum(0, 1, 2, 3, count1);

  //test single-out multi-stream
  LaCount count2 = 2000;

  la_AaddBmulC_min_multi(4, 5, 6, 7, count2);
  la_AmulBaddC_min_multi(4, 5, 6, 7, count2);
  la_AsubBmulC_min_multi(4, 5, 6, 7, count2);
  la_AmulBsubC_min_multi(4, 5, 6, 7, count2);
  la_AaddBdivC_min_multi(4, 5, 6, 7, count2);
  la_AdivBaddC_min_multi(4, 5, 6, 7, count2);
  la_AsubBdivC_min_multi(4, 5, 6, 7, count2);
  la_AdivBsubC_min_multi(4, 5, 6, 7, count2);

  la_AaddBmulC_max_multi(4, 5, 6, 7, count2);
  la_AmulBaddC_max_multi(4, 5, 6, 7, count2);
  la_AsubBmulC_max_multi(4, 5, 6, 7, count2);
  la_AmulBsubC_max_multi(4, 5, 6, 7, count2);
  la_AaddBdivC_max_multi(4, 5, 6, 7, count2);
  la_AdivBaddC_max_multi(4, 5, 6, 7, count2);
  la_AsubBdivC_max_multi(4, 5, 6, 7, count2);
  la_AdivBsubC_max_multi(4, 5, 6, 7, count2);

  la_AaddBmulC_sum_multi(4, 5, 6, 7, count2);
  la_AmulBaddC_sum_multi(4, 5, 6, 7, count2);
  la_AsubBmulC_sum_multi(4, 5, 6, 7, count2);
  la_AmulBsubC_sum_multi(4, 5, 6, 7, count2);
  la_AaddBdivC_sum_multi(4, 5, 6, 7, count2);
  la_AdivBaddC_sum_multi(4, 5, 6, 7, count2);
  la_AsubBdivC_sum_multi(4, 5, 6, 7, count2);
  la_AdivBsubC_sum_multi(4, 5, 6, 7, count2);

  //test vector-out single-stream
  LaCount count3 = 3000;

  la_AaddBmulC(0, 1, 2, 3, count3);
  la_AmulBaddC(0, 1, 2, 3, count3);
  la_AsubBmulC(0, 1, 2, 3, count3);
  la_AmulBsubC(0, 1, 2, 3, count3);
  la_AaddBdivC(0, 1, 2, 3, count3);
  la_AdivBaddC(0, 1, 2, 3, count3);
  la_AsubBdivC(0, 1, 2, 3, count3);
  la_AdivBsubC(0, 1, 2, 3, count3);
}


int main(int argc, char **argv){
  check_set_scalar();
  check_set_vec();
  check_set_spv();
  check_copy();
  check_data_op();
}
