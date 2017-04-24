//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#include "transpose/stranspose_la_core.h"

#include "la_core/la_core.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "matrix_types.h"
#include "util.h"

extern bool USE_SCRATCHPAD;
extern 
//==========================================================================
//NOTE: right now assuming a 64 KB scratchpad, so fitting a square of 
//      float precisions is around 64x64
//==========================================================================
static IDX LA_CORE_BLOCK_S = 128;

void stranspose_la_core(float *src, float *dst, IDX N, IDX M)
{
  const LaRegIdx src_mem_REG = 0;
  const LaRegIdx src_sch_REG = 1;
  const LaRegIdx dst_sch_REG = 0;
  const LaRegIdx dst_mem_REG = 1;

  if (USE_SCRATCHPAD) {
  for(IDX i=0; i<N; i+=LA_CORE_BLOCK_S) {
    for(IDX j=0; j<M; j+=LA_CORE_BLOCK_S) {
      IDX icount = MIN(LA_CORE_BLOCK_S, (N - i));
      IDX jcount = MIN(LA_CORE_BLOCK_S, (M - j));

      LaAddr src_addr = (LaAddr)(src + (i+N*j));
      LaAddr dst_addr = (LaAddr)(dst + (j+M*i));

      //transfer source data from mem to scratchpad
      la_set_vec_sp_mem(src_mem_REG, src_addr, 1,    icount, (N-icount));
      la_set_vec_sp_sch(src_sch_REG, 0,      jcount, icount, 1-(jcount*icount));
      la_copy(src_sch_REG, src_mem_REG, icount*jcount);

      //transfer dst data from scratchpad to mem
      la_set_vec_sp_sch(dst_sch_REG, 0,        1, jcount, 0);
      la_set_vec_sp_mem(dst_mem_REG, dst_addr, 1, jcount, (M-jcount));
      la_copy(dst_mem_REG, dst_sch_REG, icount*jcount);
    }
  }
}
