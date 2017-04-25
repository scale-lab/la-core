//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#include "trsm/dtrsm_la_core.h"

#include <stdbool.h>
#include <stdint.h>

#include "matrix_types.h"
#include "util.h"

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"

extern bool     DEBUG;
extern uint64_t SCRATCH_SIZE;
extern bool     USE_SCRATCH;

//==========================================================================
// INNER KERNEL, WITH SCRATCHPAD
//==========================================================================

//NOTE: right now assuming that a full row/column of either matrix can fit 
//      into the scratchpad! At a 64 KB scratchpad, this would limit us to 
//      around 2700x2700 element matrices. Clearly, we can modify the algorithm
//      to use chunks in the future
static void dtrsm_kernel_scratch(IDX j_start, IDX j_end, IDX j_max, 
  IDX k_count, bool j_increasing, bool unit, double alpha,
  double * Ain_addr, double * Xin_addr, double * Bin_addr, double *Ajj_addr)
{ 
  //used for determining the strides and skips of vectors for each iteration
  IDX j_sign = (j_increasing ? 1 : -1);
  IDX k_sign = j_sign;
  IDX i_sign = j_sign;
  IDX j_count = (j_end - j_start) * j_sign;

  IDX j_sign_jmax = (j_sign*j_max);
  IDX j_sign_jmax_plus_1 = (j_sign * (1 + j_max));
  IDX k_sign_jmax = (k_sign*j_max);

  //what registers to use for constants
  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx Alpha_REG        = 1;
  const LaRegIdx Ajj_REG          = 2;

  //registers holding input and intermediate vectors info
  const LaRegIdx Ain_mem_REG      = 3;
  const LaRegIdx Ain_sch_REG      = 4;
  const LaRegIdx Xin_mem_REG      = 5;
  const LaRegIdx Ain_Xin_sch_REG  = 6;

  //registers holding destination vector info
  const LaRegIdx Bin_mem_REG      = 3;
  const LaRegIdx Bin_sch_REG      = 4;
  const LaRegIdx Xout_mem_REG     = 5;

  //memory start addresses in the scratchpad for intermediate results
  const LaAddr BIN_SCH_ADDR      = 0;
  const LaAddr AIN_SCH_ADDR      = BIN_SCH_ADDR + k_count*sizeof(double);


  //CONFIG: load some constants into LAcore registers
  la_set_scalar_dp_reg(Zero_REG,  0.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  for (IDX j = j_start; j != j_end; j += j_sign) {
    LaVecCount  i_count       = (j_increasing ? (j) : (j_max-j-1));

    LaAddr AIN_XIN_SCH_ADDR   = AIN_SCH_ADDR + i_count*sizeof(double);

    LaAddr      Ain           = (LaAddr) Ain_addr;          
    LaAddr      Xin           = (LaAddr) Xin_addr;
    LaAddr      Bin           = (LaAddr) Bin_addr;
    LaAddr      Xout          = (LaAddr) Bin_addr;

    LaVecSkip   Ain_skip      = -i_sign*i_count;
    LaVecSkip   Xin_skip      = i_sign*(j_max-i_count);

    double      Ajj           = unit ? 1.0 : *Ajj_addr;

    if(i_count > 0) {
      //CONFIG: the input (Ain, Xin) and intermediate (Ain_Xin) vectors
      //        for the multi-sum, we need all the counts to be equal!
      la_set_vec_dp_mem(Ain_mem_REG,      Ain, i_sign, i_count, Ain_skip);
      la_set_vec_dp_sch(Ain_sch_REG,      AIN_SCH_ADDR, 1, i_count, -i_count);
      la_set_vec_dp_mem(Xin_mem_REG,      Xin, i_sign, i_count, Xin_skip);
      la_set_vec_dp_sch(Ain_Xin_sch_REG,  AIN_XIN_SCH_ADDR, 1, i_count, 0);

      //XFER: Ain (scratch) <- Ain (mem)
      //EXEC: Ain_Xin = (Ain+0)*Xin (does k_count dotprods of length i_count)
      la_copy(Ain_sch_REG, Ain_mem_REG, i_count);
      la_AaddBmulC_sum_multi(Ain_Xin_sch_REG, Ain_sch_REG, Zero_REG,
                             Xin_mem_REG, (i_count*k_count));

      //CONFIG: set the destination vector and Ajj scaling constant
      la_set_scalar_dp_reg(Ajj_REG, Ajj);
      la_set_vec_dp_mem(Bin_mem_REG, Bin, 0, 1, k_sign_jmax);
      la_set_vec_dp_sch(Bin_sch_REG, BIN_SCH_ADDR, 1, k_count, 0);
      la_set_vec_dp_mem(Xout_mem_REG, Xout, 0, 1, k_sign_jmax);

      //EXEC: Bin_sch = (Bin_mem + 0) * alpha
      //EXEC: Xout_mem = (Bin_sch - Ain_Xin) / Ajj
      la_AaddBmulC(Bin_sch_REG, Bin_mem_REG, Zero_REG, Alpha_REG, k_count); 
      la_AsubBdivC(Xout_mem_REG,
                   Bin_sch_REG, Ain_Xin_sch_REG, Ajj_REG, k_count);
    } else {
      //EXEC: Xout_mem = alpha / Ajj * (Bin_mem)
      la_set_scalar_dp_reg(Ajj_REG, alpha/Ajj);
      la_set_vec_dp_mem(Bin_mem_REG, Bin, 0, 1, k_sign_jmax);
      la_AaddBmulC(Bin_mem_REG, Bin_mem_REG, Zero_REG, Ajj_REG, k_count); 
    }

    //update pointers within the matrices (Xin never changes!)
    Ain_addr  += j_sign_jmax;
    Bin_addr  += j_sign;
    Ajj_addr  += j_sign_jmax_plus_1;
  }    
}

//==========================================================================
// INNER KERNEL, WITHOUT SCRATCHPAD
//==========================================================================

//hold a 256 KB buffere here for now, this is a bad solution...
static double tmp_buf[1 << 18];
static double tmp_buf2[1 << 18];

//NOTE: right now assuming that a full row/column of either matrix can fit 
//      into the scratchpad! At a 64 KB scratchpad, this would limit us to 
//      around 2700x2700 element matrices. Clearly, we can modify the algorithm
//      to use chunks in the future
static void dtrsm_kernel_no_scratch(IDX j_start, IDX j_end, IDX j_max, 
  IDX k_count, bool j_increasing, bool unit, double alpha,
  double * Ain_addr, double * Xin_addr, double * Bin_addr, double *Ajj_addr)
{ 
  //what registers to use for constants
  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx Alpha_REG        = 1;
  const LaRegIdx Ajj_REG          = 2;

  //registers holding input and intermediate vectors info
  const LaRegIdx Ain_mem_REG      = 3;
  const LaRegIdx Xin_mem_REG      = 5;
  const LaRegIdx Ain_Xin_mem_REG  = 6;

  //registers holding destination vector info
  const LaRegIdx Bin_mem_REG      = 3;
  const LaRegIdx Bin_tmp_mem_REG  = 4;
  const LaRegIdx Xout_mem_REG     = 5;

  //used for determining the strides and skips of vectors for each iteration
  IDX j_sign = (j_increasing ? 1 : -1);
  IDX k_sign = j_sign;
  IDX i_sign = j_sign;

  //CONFIG: load some constants into LAcore registers
  la_set_scalar_dp_reg(Zero_REG,  0.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  for (IDX j = j_start; j != j_end; j += j_sign) {
    LaVecCount  i_count       = (j_increasing ? (j) : (j_max-j-1));

    LaAddr      Ain           = (LaAddr) Ain_addr;          
    LaAddr      Xin           = (LaAddr) Xin_addr;
    LaAddr      Bin           = (LaAddr) Bin_addr;
    LaAddr      Xout          = (LaAddr) Bin_addr;

    LaVecSkip   Ain_skip      = ((-i_sign) * i_count);
    LaVecSkip   Xin_skip      = (i_sign * (j_max-i_count));

    double      Ajj           = unit ? 1.0 : *Ajj_addr;

    LaAddr      Ain_Xin_mem   = (LaAddr) tmp_buf;
    LaAddr      Bin_tmp_mem   = (LaAddr) tmp_buf2;

    if(i_count > 0){
      //CONFIG: the input (Ain, Xin) and intermediate (Ain_Xin) vectors
      //        for the multi-sum, we need all the counts to be equal!
      la_set_vec_dp_mem(Ain_mem_REG,      Ain, i_sign, i_count, Ain_skip);
      la_set_vec_dp_mem(Xin_mem_REG,      Xin, i_sign, i_count, Xin_skip);
      la_set_vec_dp_mem(Ain_Xin_mem_REG,  Ain_Xin_mem, 1, k_count, 0);

      //XFER: Ain (scratch) <- Ain (mem)
      //EXEC: Ain_Xin = (Ain+0)*Xin (does k_count dotprods of length i_count)
      la_AaddBmulC_sum_multi(Ain_Xin_mem_REG, Ain_mem_REG, Zero_REG, 
                             Xin_mem_REG, (i_count*k_count));

      //CONFIG: set the destination vector and Ajj scaling constant
      la_set_scalar_dp_reg(Ajj_REG, Ajj);
      la_set_vec_dp_mem(Bin_mem_REG, Bin, 0, 1, k_sign*j_max);
      la_set_vec_dp_mem(Bin_tmp_mem_REG, Bin_tmp_mem, 1, k_count, 0);
      la_set_vec_dp_mem(Xout_mem_REG, Xout, 0, 1, k_sign*j_max);

      //EXEC: Bin_sch = (Bin_mem + 0) * alpha
      //EXEC: Xout_mem = (Bin_sch - Ain_Xin) / Ajj
      la_AaddBmulC(Bin_tmp_mem_REG, Bin_mem_REG, Zero_REG, Alpha_REG, k_count); 
      la_AsubBdivC(Xout_mem_REG, 
                   Bin_tmp_mem_REG, Ain_Xin_mem_REG, Ajj_REG, k_count);
    } else {
      //EXEC: Xout_mem = alpha / Ajj * (Bin_mem)
      la_set_scalar_dp_reg(Ajj_REG, alpha/Ajj);
      la_set_vec_dp_mem(Bin_mem_REG, Bin, 0, 1, (k_sign*j_max));
      la_AaddBmulC(Bin_mem_REG, Bin_mem_REG, Zero_REG, Ajj_REG, k_count); 
    }

    //update pointers within the matrices (Xin never changes!)
    Ain_addr  += (j_sign * j_max);
    Bin_addr  += (j_sign);
    Ajj_addr  += (j_sign * (1 + j_max)); 
  }
}

//==========================================================================
// LEFT LOWER KERNELS (traverse top-left to bottom right)
//==========================================================================

//A row-major (MxM), B col-major (MxP). Solves for X in place of B
static void dtrsm_left_lower(double *A, double *B, IDX M, IDX P,
  IDX jstart, IDX jcount, IDX kstart, IDX kcount, bool unit, double alpha)
{
  IDX jend = jstart + jcount;
  IDX jmax = M;
  bool j_increasing = true;

  double * X        = B;
  double * Ain_addr = &(IDXR(A, jstart, 0,      M, M));
  double * Xin_addr = &(IDXC(X, 0,      kstart, M, P));
  double * Bin_addr = &(IDXC(B, jstart, kstart, M, P));
  double * Ajj_addr = &(IDXR(A, jstart, jstart, M, M));

  if(USE_SCRATCH) {
    dtrsm_kernel_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);   
  } else {            
    dtrsm_kernel_no_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);    
  }
}

//==========================================================================
// RIGHT UPPER KERNELS (traverse top-left to bottom right)
//==========================================================================

//A col-major (MxM), B row-major (PxM). Solves for X in place of B
static void dtrsm_right_upper(double *A, double *B, IDX M, IDX P,
  IDX jstart, IDX jcount, IDX kstart, IDX kcount, bool unit, double alpha)
{
  IDX jend = jstart + jcount;
  IDX jmax = M;
  bool j_increasing = true;

  double * X        = B;
  double * Ain_addr = &(IDXC(A, 0,       jstart, M, M));
  double * Xin_addr = &(IDXR(X, kstart, 0,       P, M));
  double * Bin_addr = &(IDXR(B, kstart, jstart,  P, M));
  double * Ajj_addr = &(IDXC(A, jstart, jstart,  M, M));

  if(USE_SCRATCH) {
    dtrsm_kernel_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);   
  } else {  
    dtrsm_kernel_no_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);    
  }    
}

//==========================================================================
// LEFT UPPER KERNELS (traverse bottom-right to top-left)
//==========================================================================

//A row-major (MxM), B col-major (MxP). Solves for X in place of B
static void dtrsm_left_upper(double *A, double *B, IDX M, IDX P,
  IDX j, IDX jcount, IDX k, IDX kcount, bool unit, double alpha)
{
  IDX jstart        = j + jcount - 1;
  IDX jend          = j - 1;
  IDX jmax          = M;
  IDX kstart        = k + kcount - 1;
  bool j_increasing = false;

  double * X        = B;
  double * Ain_addr = &(IDXR(A, jstart,   (jmax-1), M, M));
  double * Xin_addr = &(IDXC(X, (jmax-1), kstart,   M, P));
  double * Bin_addr = &(IDXC(B, jstart,   kstart, M, P));
  double * Ajj_addr = &(IDXR(A, jstart,   jstart, M, M));

  if(USE_SCRATCH) {
    dtrsm_kernel_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);   
  } else {   
    dtrsm_kernel_no_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);    
  }  
}

//==========================================================================
// RIGHT LOWER KERNELS (traverse bottom-right to top-left)
//==========================================================================

//A col-major (MxM), B row-major (PxM). Solves for X in place of B
static void dtrsm_right_lower(double *A, double *B, IDX M, IDX P,
  IDX j, IDX jcount, IDX k, IDX kcount, bool unit, double alpha)
{
  IDX jstart        = j + jcount - 1;
  IDX jend          = j - 1;
  IDX jmax          = M;
  IDX kstart        = k + kcount - 1;
  bool j_increasing = false;

  double * X        = B;
  double * Ain_addr = &(IDXC(A, (jmax-1), jstart,   M, M));
  double * Xin_addr = &(IDXR(X, kstart,   (jmax-1), P, M));
  double * Bin_addr = &(IDXR(B, kstart,   jstart,   P, M));
  double * Ajj_addr = &(IDXC(A, jstart,   jstart,   M, M));

  if(USE_SCRATCH) {
    dtrsm_kernel_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);   
  } else {   
    dtrsm_kernel_no_scratch(jstart, jend, jmax, kcount, j_increasing, unit,
      alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);    
  }      
}

//==========================================================================
// Select the approprate Sub-TRSM algorithm and Panelize it
//
// The below algorithm calls the subroutine in small 'blocks' in order to 
// attempt to improve caching performance
//
// A is (MxM) matrix. B is (MxP) if (right==false) and (PxM) if (right==true)
//==========================================================================

//J_block doesn't matter
#define J_BLOCK 65536
#define K_BLOCK 256

void dtrsm_la_core(double *A, double *B, IDX M, IDX P,
  bool right, bool upper, bool unit, double alpha)
{
  if ((upper && right) || (!upper && !right)) {
    for (IDX j = 0; j< M; j += J_BLOCK) {
      IDX jcount = MIN(J_BLOCK, (M - j));
      for (IDX k = 0; k < P; k += K_BLOCK) {
        IDX kcount = MIN(K_BLOCK, (P - k));
        if (upper) {
          dtrsm_right_upper(A, B, M, P, j, jcount, k, kcount, unit, alpha);
        }
        else {
          dtrsm_left_lower(A, B, M, P, j, jcount, k, kcount, unit, alpha);
        }
      }
    }
  }
  else {
    IDX j = M - (M % J_BLOCK);
    IDX kstart = P - (P % K_BLOCK);
    while (true) {
      IDX k = kstart;
      while (true) {
        IDX jcount = MIN(J_BLOCK, (M - j));
        IDX kcount = MIN(K_BLOCK, (P - k));
        if (upper) {
          dtrsm_left_upper(A, B, M, P, j, jcount, k, kcount, unit, alpha);
        }
        else {
          dtrsm_right_lower(A, B, M, P, j, jcount, k, kcount, unit, alpha);
        }
        if (k == 0) {
          break;
        }
        k -= K_BLOCK;
      }
      if (j == 0) {
        break;
      }
      j -= J_BLOCK;
    }
  }
}













/*
void dtrsm_left_lower_fast(double *A, double *B, IDX M, IDX P,
  bool unit, double alpha)
{

  const IDX DPSIZE = sizeof(double);
  const LaAddr ACCUM_SCH_ADR = 0;

  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx Accum_sch_REG    = 1;
  const LaRegIdx Accum_j_reg_REG  = 2;
  const LaRegIdx Aji_Xik_mem_REG  = 3;
  const LaRegIdx Xjk_reg_REG      = 4;

  double Accum_j;

  la_set_vec_adr_dp_mem(Accum_j_reg_REG, (LaAddr)&Accum_j);
  la_set_scalar_dp_reg(Zero_REG, 0.0);

  for(IDX k=0; k<P; ++k) {
    //zero the accumulators
    la_set_vec_adr_dp_sch(Accum_sch_REG, ACCUM_SCH_ADR);
    la_copy(Accum_sch_REG, Zero_REG, M);

    //solve each item in B column at a time
    for(IDX j=0; j<M; ++j) {
      la_set_vec_adr_dp_sch(Accum_sch_REG, ACCUM_SCH_ADR+DPSIZE*j);
      la_copy(Accum_j_reg_REG, Accum_sch_REG, 1);

      double Xjk = alpha*IDXC(B, j, k, M, P) - Accum_j;
      Xjk = unit ? Xjk : Xjk/IDXC(A, j, j, M, M);
      IDXC(B, j, k, M, P) = Xjk;

      la_set_scalar_dp_reg(Xjk_reg_REG, Xjk);
      la_set_vec_adr_dp_mem(Aji_Xik_mem_REG, (LaAddr)&IDXC(A, j+1, j, M, M));
      la_set_vec_adr_dp_sch(Accum_sch_REG, ACCUM_SCH_ADR+DPSIZE*(j+1));
      la_AmulBaddC(Accum_sch_REG, 
                   Xjk_reg_REG, Aji_Xik_mem_REG, ACCUM_SCH_ADRREG, M-(j+1));
    }
  }
}
*/


void dtrsm_left_lower_fast(double *A, double *B, IDX M, IDX P,
  bool unit, double alpha)
{
  const IDX DPSIZE = sizeof(double);
  const LaAddr ACCUM_SCH_ADR      = 0;

  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx Alpha_REG        = 1;
  const LaRegIdx Minus_One_REG    = 2;
  const LaRegIdx Ajj_reg_REG      = 3;
  const LaRegIdx Accum_sch_REG    = 4;
  const LaRegIdx B0k_mem_REG      = 5;
  const LaRegIdx Aji_Xik_mem_REG  = 6;
  const LaRegIdx Xjk_reg_REG      = 7;

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  for(IDX k=0; k<P; ++k) {
    //precompute alpha*Bjk
    la_set_vec_adr_dp_sch(Accum_sch_REG, ACCUM_SCH_ADR);
    la_set_vec_adr_dp_mem(B0k_mem_REG, (LaAddr)&IDXC(B, 0, k, M, P));
    la_AmulBaddC(Accum_sch_REG, Alpha_REG, B0k_mem_REG, Zero_REG, M);

    //solve each item in B column at a time
    for(IDX j=0; j<M; ++j) {
      LaAddr j_sch_adr = ACCUM_SCH_ADR+DPSIZE*j;
      la_set_scalar_dp_reg(Ajj_reg_REG, unit ? -1.0 : -IDXC(A, j, j, M, M));
      la_set_vec_adr_dp_sch(Accum_sch_REG, j_sch_adr);
      la_AaddBdivC(Accum_sch_REG, Accum_sch_REG, Zero_REG, Ajj_reg_REG, 1);

      la_set_scalar_dp_sch(Xjk_reg_REG, j_sch_adr);
      la_set_vec_adr_dp_mem(Aji_Xik_mem_REG, (LaAddr)&IDXC(A, j+1, j, M, M));
      la_set_vec_adr_dp_sch(Accum_sch_REG, j_sch_adr+DPSIZE);
      la_AmulBaddC(Accum_sch_REG, 
                   Xjk_reg_REG, Aji_Xik_mem_REG, Accum_sch_REG, M-(j+1));
    }

    la_set_scalar_dp_reg(Minus_One_REG, -1.0);
    la_set_vec_adr_dp_sch(Accum_sch_REG, ACCUM_SCH_ADR);
    la_set_vec_adr_dp_mem(B0k_mem_REG, (LaAddr)&IDXC(B, 0, k, M, P));
    la_AmulBaddC(B0k_mem_REG, Accum_sch_REG, Minus_One_REG, Zero_REG, M);
  }
}









void dtrsm_la_core_fast(double *A, double *B, IDX M, IDX P,
  bool right, bool upper, bool unit, double alpha)
{
  if ((upper && right) || (!upper && !right)) {
    if (upper) {
      printf("fast dtrsm algo not implemented yet\n");
    }
    else {
      dtrsm_left_lower_fast(A, B, M, P, unit, alpha);
    }
  }
  else {
    printf("fast dtrsm algo not implemented yet\n");
  }
}
