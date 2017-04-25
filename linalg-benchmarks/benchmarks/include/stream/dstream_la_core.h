//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of the HPCC STREAM benchmark
// http://www.cs.virginia.edu/~mccalpin/papers/bandwidth/node2.html
// http://www.cs.virginia.edu/stream/ref.html
//==========================================================================

#ifndef __DSTREAM_LA_CORE_H__
#define __DSTREAM_LA_CORE_H__

#include "matrix_types.h"

void dstream_copy_la_core(double *A, double *B, IDX count);
void dstream_scale_la_core(double *A, double *B, double q, IDX count);
void dstream_sum_la_core(double *A, double *B, double *C, IDX count);
void dstream_triad_la_core(double *A, double *B, double q, 
                           double *C, IDX count);


#endif // __DSTREAM_LA_CORE_H__
