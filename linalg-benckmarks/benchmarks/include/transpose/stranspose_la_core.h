//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of high-performance matrix transpose
//==========================================================================

#ifndef __STRANSPOSE_LA_CORE_H__
#define __STRANSPOSE_LA_CORE_H__

#include "matrix_types.h"

//==========================================================================
// dst = transpose(src), where src is NxM matrix in memory
//==========================================================================
void stranspose_la_core(float *src, float *dst, IDX N, IDX M);

#endif // __STRANSPOSE_LA_CORE_H__
