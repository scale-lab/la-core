//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of sparse DGEMV
//==========================================================================

#ifndef __DGEMV_SPV_LA_CORE_H__
#define __DGEMV_SPV_LA_CORE_H__

#include <stdint.h>

#include "matrix_types.h"

//==========================================================================
//only reads A_ptr as row-major
//N = height of A
//M = width of A, height of b and x
//==========================================================================
void dgemv_spv_la_core(double *A_data_ptr, uint32_t *A_row_ptr,
  uint32_t *A_col_ptr, double *b, double *x, IDX N, IDX M);

#endif // __DGEMV_SPV_LA_CORE_H__
