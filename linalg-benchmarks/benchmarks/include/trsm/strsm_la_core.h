//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of BLAS-3 STRSM function
//==========================================================================

#ifndef __STRSM_LA_CORE_H__
#define __STRSM_LA_CORE_H__

#include "matrix_types.h"

//==========================================================================
// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
//==========================================================================
void strsm_la_core(float *A, float *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, float alpha);

#endif // __STRSM_LA_CORE_H__