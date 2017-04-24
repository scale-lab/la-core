//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of Linear System Solve using LU Decomp
//==========================================================================

#ifndef __DLU_SOLVE_LA_CORE_H__
#define __DLU_SOLVE_LA_CORE_H__

#include "matrix_types.h"

void dlu_solve_la_core(double *A, double *B, IDX N, IDX P);

//assumes B to be 1 column vector, A in row-major order
void dlu_solve_la_core_fast(double *A, double *B, IDX N);

#endif // __DLU_SOLVE_LA_CORE_H__
