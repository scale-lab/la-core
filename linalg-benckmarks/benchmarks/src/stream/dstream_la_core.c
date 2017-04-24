//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of the HPCC STREAM benchmark
// http://www.cs.virginia.edu/~mccalpin/papers/bandwidth/node2.html
// http://www.cs.virginia.edu/stream/ref.html
//==========================================================================

#include "stream/dstream_la_core.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "matrix_types.h"
#include "util.h"

extern uint64_t SCRATCH_SIZE;
extern bool USE_SCRATCH;

//==========================================================================

// a(i) = b(i)
// NOTE: no use in using scratchpad here
void dstream_copy_la_core(double *A, double *B, IDX count)
{
  const LaRegIdx A_REG = 1;
  const LaRegIdx B_REG = 2;

  la_set_vec_adr_dp_mem(A_REG, (LaAddr)A);
  la_set_vec_adr_dp_mem(B_REG, (LaAddr)B);

  la_copy(A_REG, B_REG, count);
}

// a(i) = q*b(i)
void dstream_scale_la_core(double *A, double *B, double q, IDX count)
{
  const LaRegIdx q_REG = 0;
  const LaRegIdx A_REG = 1;
  const LaRegIdx B_REG = 2;
  const LaRegIdx Zero_REG = 3;

  la_set_scalar_dp_reg(q_REG, q);
  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_vec_adr_dp_mem(A_REG, (LaAddr)A);
  la_set_vec_adr_dp_mem(B_REG, (LaAddr)B);

  //A = (0+B)*q
  la_AaddBmulC(A_REG, Zero_REG, B_REG, q_REG, count);
}

// a(i) = b(i) + c(i)
void dstream_sum_la_core(double *A, double *B, double *C, IDX count)
{
  const LaRegIdx A_REG = 0;
  const LaRegIdx B_REG = 1;
  const LaRegIdx C_REG = 2;
  const LaRegIdx One_REG = 3;

  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_vec_adr_dp_mem(A_REG, (LaAddr)A);
  la_set_vec_adr_dp_mem(B_REG, (LaAddr)B);
  la_set_vec_adr_dp_mem(C_REG, (LaAddr)C);

  //A = (B+C)*1
  la_AaddBmulC(A_REG, B_REG, C_REG, One_REG, count);
}

// a(i) = b(i) + q*c(i)
void dstream_triad_la_core(double *A, double *B, double q, 
                           double *C, IDX count)
{
  const LaRegIdx A_REG = 0;
  const LaRegIdx B_REG = 1;
  const LaRegIdx C_REG = 2;
  const LaRegIdx q_REG = 3;

  la_set_scalar_dp_reg(q_REG, q);
  la_set_vec_adr_dp_mem(A_REG, (LaAddr)A);
  la_set_vec_adr_dp_mem(B_REG, (LaAddr)B);
  la_set_vec_adr_dp_mem(C_REG, (LaAddr)C);

  la_AmulBaddC(A_REG, C_REG, q_REG, B_REG, count);
}
