//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#include "transpose/dtranspose_la_core.h"

#include "la_core/la_core.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "matrix_types.h"
#include "util.h"

void dcopy_la_core(double *src, double *dst, IDX N, IDX M)
{
  const LaRegIdx src_mem_REG = 0;
  const LaRegIdx dst_mem_REG = 1;

  la_set_vec_addr_dp_mem(src_mem_REG, (LAAddr)src);
  la_set_vec_addr_dp_mem(dst_mem_REG, (LAAddr)dst);
  la_copy(dst_mem_REG, src_mem_REG, N*M);
}