//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of Complex double-precision FFT of
// arbitrary-length vectors
//==========================================================================

#ifndef __DFFT_LA_CORE_H__
#define __DFFT_LA_CORE_H__

#include "matrix_types.h"

typedef struct Complex {
  double re;
  double im;
} Complex;

void dfft_la_core(Complex *A, IDX size);

#endif // __DFFT_LA_CORE_H__
