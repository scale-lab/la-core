//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Implementations of BLAS-3 TRSM functions
//==========================================================================

#ifndef __TRSM_H__
#define __TRSM_H__

#include <intrin.h>

#include "matrix_utilities.h"

//==========================================================================
// Intel AVX Optimized TRSM
//==========================================================================

void strsm_avx(float *A, float *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, float alpha);

void dtrsm_avx(double *A, double *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, double alpha);

//==========================================================================
// Generic Platform Optimised STRSM
//==========================================================================

void strsm_generic(float *A, float *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, float alpha);

void dtrsm_generic(double *A, double *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, double alpha);

//==========================================================================
// Generic Platform Optimised STRSM
//==========================================================================

void strsm_lacore(float *A, float *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, float alpha);

void dtrsm_lacore(double *A, double *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, double alpha);

#endif //__TRSM_H__