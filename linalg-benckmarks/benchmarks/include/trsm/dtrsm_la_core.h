//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#ifndef __DTRSM_LA_CORE_H__
#define __DTRSM_LA_CORE_H__

#include <stdbool.h>

#include "matrix_types.h"

//==========================================================================
// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
//==========================================================================
void dtrsm_la_core(double *A, double *B, IDX M, IDX P,
  bool right, bool upper, bool unit, double alpha);

void dtrsm_la_core_fast(double *A, double *B, IDX M, IDX P,
  bool right, bool upper, bool unit, double alpha);

#endif // __DTRSM_LA_CORE_H__
