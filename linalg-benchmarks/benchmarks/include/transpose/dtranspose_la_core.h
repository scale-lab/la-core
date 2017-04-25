//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of high-performance matrix transpose
//==========================================================================

#ifndef __DTRANSPOSE_LA_CORE_H__
#define __DTRANSPOSE_LA_CORE_H__

#include "matrix_types.h"

//==========================================================================
// dst = transpose(src), where src is NxM matrix in memory
//==========================================================================
void dtranspose_la_core(double *src, double *dst, IDX N, IDX M);

#endif // __DTRANSPOSE_LA_CORE_H__
