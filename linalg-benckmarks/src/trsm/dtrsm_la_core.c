//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#include "trsm.h"

#include "matrix_utilities.h"

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"

//NOTE: right now assuming that a full row/column of either matrix can fit 
//      into the scratchpad! At a 64 KB scratchpad, this would limit us to 
//      around 2700x2700 element matrices. Clearly, we can modify the algorithm
//      to use chunks in the future
static void dtrsm_kernel(IDX j_start, IDX j_end, IDX j_max, 
  IDX k_count, bool j_increasing, bool unit, double alpha,
  double * Ain_addr, double * Xin_addr, double * Bin_addr, double *Ajj_addr)
{ 
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
  const LaAddr AIN_SCH_ADDR      = BIN_SCH_ADDR + j_end*sizeof(double);
  const LaAddr AIN_XIN_SCH_ADDR  = AIN_SCH_ADDR + j_end*sizeof(double);

  //used for determining the strides and skips of vectors for each iteration
  IDX j_sign = (j_increasing ? 1 : -1);
  IDX k_sign = j_sign;
  IDX i_sign = j_sign;

  //CONFIG: load some constants into LAcore registers
  la_set_scalar_dp_reg(Zero_REG,  0.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  for (IDX j = j_start; j != j_end; j += j_dir_sign) {

    //update X in place of B
    double *    Xout_addr     = Bin_addr;

    //i_count is size of Ain*Xin dot product
    LaVecCount  i_count       = (j_increasing ? (j_max-j) : (j));

    LaAddr      Ain           = (LaAddr) Ain_addr;          
    LaVecStride Ain_stride    = i_sign;                     //const
    LaVecCount  Ain_count     = i_count;
    LaVecSkip   Ain_skip      = ((-i_sign) * i_count);

    LaAddr      Xin           = (LaAddr) Xin_addr;          //const
    LaVecStride Xin_stride    = i_sign;                     //const    
    LaVecCount  Xin_count     = i_count;
    LaVecSkip   Xin_skip      = (i_sign * (j_max-i_count));

    LaAddr      Bin           = (LaAddr) Bin_addr;
    LaVecStride Bin_stride    = 0;                          //const
    LaVecCount  Bin_count     = 1;                          //const
    LaVecSkip   Bin_skip      = (k_sign * (j_max));         //const

    LaAddr      Xout          = (LaAddr) Xout_addr;
    LaVecStride Xout_stride   = 0;                          //const
    LaVecCount  Xout_count    = 1;                          //const
    LaVecSkip   Xout_skip     = (k_sign * (j_max));         //const

    double      Ajj           = unit ? 1.0 : *Ajj_addr;

    //CONFIG: the input (Ain, Xin) and intermediate (Ain_Xin) vectors
    //        for the multi-sum, we need all the counts to be equal!
    la_set_vec_dp_mem(Ain_mem_REG,      Ain, Ain_stride, Ain_count, Ain_skip);
    la_set_vec_dp_sch(Ain_sch_REG,      AIN_SCH_ADDR);
    la_set_vec_dp_mem(Xin_mem_REG,      Xin, Xin_stride, Xin_count, Xin_skip);
    la_set_vec_dp_sch(Ain_Xin_sch_REG,  AIN_XIN_SCH_ADDR, i_count);

    //XFER: Ain (scratch) <- Ain (mem)
    //EXEC: Ain_Xin = (Ain+0)*Xin (does k_count dotprods of length i_count)
    la_copy(Ain_sch_REG, Ain_mem_REG, i_count);
    la_AaddBmulC_sum_multi(Ain_Xin_sch_REG, Ain_sch_REG, Zero_REG, Xin_mem_REG, 
                       (i_count*k_count));

    //CONFIG: set the destination vector and Ajj scaling constant
    set_scalar_dp_reg(Ajj_REG, Ajj);
    la_set_vec_dp_mem(Bin_mem_REG, Bin, Bin_stride, Bin_count, Bin_skip);
    la_set_vec_dp_mem(Bin_sch_REG, BIN_SCH_ADDR);
    la_set_vec_dp_sch(Xout_mem_REG, Xout, Xout_stride, Xout_count, Xout_skip);

    //EXEC: Bin_sch = (Bin_mem + 0) * alpha
    //EXEC: Xout_mem = (Bin_sch - Ain_Xin) / Ajj
    la_AaddBmulC(Bin_sch_REG, Bin_mem_REG, Zero_REG, Alpha_REG, k_count); 
    la_AsubBdivC(Xout_mem_REG, Bin_sch_REG, Ain_Xin_sch_REG, Ajj_REG, k_count);

    //update pointers within the matrices (Xin never changes!)
    Ain_addr  += (j_dir_sign * j_max);
    Bin_addr  += (j_dir_sign);
    Ajj_addr  += (j_dir_sign * (1 + j_max));
  }    
}

//==========================================================================
// LEFT LOWER KERNELS (traverse top-left to bottom right)
//==========================================================================

//A row-major (MxM), B col-major (MxP). Solves for X in place of B
static void dtrsm_left_lower(double *A, double *B, IDX M, IDX P,
  IDX row_top, IDX row_count, IDX col_left, IDX col_count, bool unit, 
  double alpha)
{
  IDX j_start       = row_top;
  IDX j_end         = row_top + row_count;
  IDX j_max         = M;
  IDX k_start       = col_left;
  IDX k_count       = col_count;
  bool j_increasing = true;

  double * X        = B;
  double * Ain_addr = &(IDXR(A, j_start, 0,       M, M));
  double * Xin_addr = &(IDXC(X, 0,       k_start, M, P));
  double * Bin_addr = &(IDXC(B, j_start, k_start, M, P));
  double * Ajj_addr = &(IDXR(A, j_start, j_start, M, M));

  dtrsm_kernel(j_start, j_end, j_max, k_count, j_increasing, unit,
    alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);                    
}

//==========================================================================
// RIGHT UPPER KERNELS (traverse top-left to bottom right)
//==========================================================================

//A col-major (MxM), B row-major (PxM). Solves for X in place of B
static void dtrsm_right_upper(double *A, double *B, IDX M, IDX P,
  IDX row_top, IDX row_count, IDX col_left, IDX col_count, bool unit,
  double alpha)
{
  IDX j_start       = col_left;
  IDX j_end         = col_left + col_count;
  IDX j_max         = M;
  IDX k_start       = row_top;
  IDX k_count       = row_count;
  bool j_increasing = true;

  double * X        = B;
  double * Ain_addr = &(IDXC(A, 0,       j_start, M, M));
  double * Xin_addr = &(IDXR(X, k_start, 0,       P, M));
  double * Bin_addr = &(IDXR(B, k_start, j_start, P, M));
  double * Ajj_addr = &(IDXC(A, j_start, j_start, M, M));

  dtrsm_kernel(j_start, j_end, j_max, k_count, j_increasing, unit,
    alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);      
}

//==========================================================================
// LEFT UPPER KERNELS (traverse bottom-right to top-left)
//==========================================================================

//A row-major (MxM), B col-major (MxP). Solves for X in place of B
static void dtrsm_left_upper(double *A, double *B, IDX M, IDX P,
  IDX row_top, IDX row_count, IDX col_left, IDX col_count, bool unit,
  double alpha)
{
  IDX j_start       = row_top + row_count - 1;
  IDX j_end         = row_top - 1;
  IDX j_max         = M;
  IDX k_start       = col_left + col_count - 1;
  IDX k_count       = col_count - 1;
  bool j_increasing = false;

  double * X        = B;
  double * Ain_addr = &(IDXR(A, j_start,   (j_max-1), M, M));
  double * Xin_addr = &(IDXC(X, (j_max-1), k_start,   M, P));
  double * Bin_addr = &(IDXC(B, j_start,   k_start, M, P));
  double * Ajj_addr = &(IDXR(A, j_start,   j_start, M, M));

  dtrsm_kernel(j_start, j_end, j_max, k_count, j_increasing, unit,
    alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);   
}

//==========================================================================
// RIGHT LOWER KERNELS (traverse bottom-right to top-left)
//==========================================================================

//A row-major (MxM), B col-major (PxM). Solves for X in place of B
static void dtrsm_right_lower(double *A, double *B, IDX M, IDX P,
  IDX row_top, IDX row_count, IDX col_left, IDX col_count, bool unit,
  double alpha)
{
  IDX j_start       = col_left + col_count - 1;
  IDX j_end         = col_count - 1;
  IDX j_max         = M;
  IDX k_start       = row_top + row_count - 1;
  IDX k_count       = row_top - 1;
  bool j_increasing = false;

  double * X        = B;
  double * Ain_addr = &(IDXC(A, (j_max-1), j_start,   M, M));
  double * Xin_addr = &(IDXR(X, k_start,   (j_max-1), P, M));
  double * Bin_addr = &(IDXR(B, k_start,   j_start,   P, M));
  double * Ajj_addr = &(IDXC(A, j_start,   j_start,   M, M));

  dtrsm_kernel(j_start, j_end, j_max, k_count, j_increasing, unit,
    alpha, Ain_addr, Xin_addr, Bin_addr, Ajj_addr);   
}

//==========================================================================
// Select the approprate Sub-TRSM algorithm and Panelize it
//
// The below algorithm calls the subroutine in small 'blocks' in order to 
// attempt to improve caching performance
//
// A is (MxM) matrix. B is (MxP) if (right==false) and (PxM) if (right==true)
//==========================================================================

void dtrsm_la_core(double *A, double *B, IDX M, IDX P,
  bool right, bool upper, bool unit, double alpha)
{
  if ((upper && right) || (!upper && !right)) {
    for (IDX row_topleft = 0; row_topleft < B_rows; row_topleft += BLOCK_D) {
      IDX row_count = MIN(BLOCK_D, (B_rows - row_topleft));
      for (IDX col_topleft = 0; col_topleft < B_cols; col_topleft += BLOCK_D) {
        IDX col_count = MIN(BLOCK_D, (B_cols - col_topleft));
        if (upper) {
          dtrsm_right_upper(A, B, M, B_rows, B_cols,
            row_topleft, row_count, col_topleft, col_count, unit, alpha);
        }
        else {
          dtrsm_left_lower(A, B, M, B_rows, B_cols,
            row_topleft, row_count, col_topleft, col_count, unit, alpha);
        }
      }
    }
  }
  else {
    IDX i = B_rows - (B_rows % BLOCK_D);
    IDX jstart = B_cols - (B_cols % BLOCK_D);
    while (true) {
      IDX j = jstart;
      while (true) {
        IDX icount = MIN(BLOCK_D, (B_rows - i));
        IDX jcount = MIN(BLOCK_D, (B_cols - j));
        if (upper) {
          dtrsm_left_upper(A, B, M, B_rows, B_cols,
            i, icount, j, jcount, unit, alpha);
        }
        else {
          dtrsm_right_lower(A, B, M, B_rows, B_cols,
            i, icount, j, jcount, unit, alpha);
        }
        if (j == 0) {
          break;
        }
        j -= BLOCK_D;
      }
      if (i == 0) {
        break;
      }
      i -= BLOCK_D;
    }
  }
}