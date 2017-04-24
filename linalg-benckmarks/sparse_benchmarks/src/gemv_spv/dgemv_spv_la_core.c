//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
//==========================================================================

#include "gemv_spv/dgemv_spv_la_core.h"

#include <assert.h>
#include <stdbool.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_set_spv.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "matrix_types.h"
#include "util.h"

extern bool     DEBUG;
extern uint64_t SCRATCH_SIZE;

//A in row major, b,x are column vectors
void dgemv_spv_la_core(double *A_data_ptr, uint32_t *A_col_ptr, 
  uint32_t *A_row_ptr, double *b, double *x, IDX N, IDX M)
{
  const LaRegIdx Zero_REG  = 0;
  const LaRegIdx A_mem_REG = 1;
  const LaRegIdx b_mem_REG = 2;
  const LaRegIdx b_sch_REG = 3;
  const LaRegIdx x_mem_REG = 4;
  const LaRegIdx x_sch_REG = 5;

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  //NOTE: idx/jdx are swapped here. i messed up the API.
  la_set_spv_nrm_dp_mem(A_mem_REG, 
    (LaAddr)A_data_ptr, (LaAddr)A_row_ptr, (LaAddr)A_col_ptr, N, M, 0);
  la_set_vec_dp_mem(b_mem_REG, (LaAddr)b,      1, M, 0);
  la_set_vec_dp_sch(b_sch_REG, 0,              1, M, -M);
  la_set_vec_dp_mem(x_mem_REG, (LaAddr)x,      1, N, 0);
  la_set_vec_dp_sch(x_sch_REG, SCRATCH_SIZE/2, 1, N, 0);
  
  la_copy(b_sch_REG, b_mem_REG, M);
  la_AmulBaddC_sum_multi(x_sch_REG, A_mem_REG, b_sch_REG, Zero_REG, M*N);
  la_copy(x_mem_REG, x_sch_REG, N);
}
