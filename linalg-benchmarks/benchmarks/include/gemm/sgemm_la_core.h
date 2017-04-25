//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of BLAS-3 SGEMM function
//==========================================================================

#ifndef __SGEMM_LA_CORE_H__
#define __SGEMM_LA_CORE_H__

#include "matrix_types.h"

//==========================================================================
// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
//==========================================================================
void sgemm_la_core(float *C, float *A, float *B, float alpha, float beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount);

#endif // __SGEMM_LA_CORE_H__