//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#ifndef __DGEMM_LA_CORE_H__
#define __DGEMM_LA_CORE_H__

#include "matrix_types.h"

//==========================================================================
// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
//==========================================================================
void dgemm_la_core(double *C, double *A, double *B, double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount);

#endif // __DGEMM_LA_CORE_H__