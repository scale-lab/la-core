//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of BLAS-3 DTRSM function
//
// 2 variations are provided: 1 that uses scratchpad and one that doesn't. 
// The purpose of this is for design-space exploration.
//==========================================================================

#include "transpose/dtranspose_la_core.h"

#include <stdbool.h>
#include <math.h>

#include "la_core/la_core.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "matrix_types.h"
#include "util.h"

extern bool     DEBUG;
extern uint64_t SCRATCH_SIZE;
extern bool     USE_SCRATCH_FOR_TRANSPOSE;

//==========================================================================

static void dtranspose_la_core_scratch(double *src, double *dst, IDX N, IDX M)
{
  const LaRegIdx src_mem_REG = 0;
  const LaRegIdx src_sch_REG = 1;
  const LaRegIdx dst_sch_REG = 0;
  const LaRegIdx dst_mem_REG = 1;

  IDX BLOCK = (IDX)(log2((double)SCRATCH_SIZE)/(double)sizeof(double));

  for(IDX i=0; i<N; i+=BLOCK) {
    for(IDX j=0; j<M; j+=BLOCK) {
      IDX icount = MIN(BLOCK, (N - i));
      IDX jcount = MIN(BLOCK, (M - j));

      LaAddr src_addr = (LaAddr)(src + (i+N*j));
      LaAddr dst_addr = (LaAddr)(dst + (j+M*i));

      //transfer source data from mem to scratchpad
      la_set_vec_dp_mem(src_mem_REG, src_addr, 1, icount, (N-icount));
      la_set_vec_dp_sch(src_sch_REG, 0, jcount, icount, 1-(jcount*icount));
      la_copy(src_sch_REG, src_mem_REG, icount*jcount);

      //transfer dst data from scratchpad to mem
      la_set_vec_dp_sch(dst_sch_REG, 0, 1, jcount, 0);
      la_set_vec_dp_mem(dst_mem_REG, dst_addr, 1, jcount, (M-jcount));
      la_copy(dst_mem_REG, dst_sch_REG, icount*jcount);
    }
  }
}

//==========================================================================

static void dtranspose_la_core_no_scratch(double *src, double *dst, IDX N, 
  IDX M)
{
  for(IDX i=0; i<N; i+=1) {
    for(IDX j=0; j<M; j+=1) {
      dst[j+M*i] = src[i+N*j];
    }
  }
}

//==========================================================================

void dtranspose_la_core(double *src, double *dst, IDX N, IDX M) 
{
  if (USE_SCRATCH_FOR_TRANSPOSE) {
    dtranspose_la_core_scratch(src, dst, N, M);
  } else {
    dtranspose_la_core_no_scratch(src, dst, N, M);
  }
}
