//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//==========================================================================

#ifndef __MATRIX_TYPES_H__
#define __MATRIX_TYPES_H__

#include <math.h>
#include <stdint.h>

//==========================================================================
// MATRIX DIMENSIONS AND INDEXES
//==========================================================================

//matrix dimension, index into that dimension
typedef int32_t IDX;

//index into an array that is stores as column or row major format in memory
//I is row, J is col, R is total rows, C is total cols in matrix. (0, 0) is 
//the top-left corner of matrix
#define IDXC(A, I, J, R, C) A[(J)*(R)+(I)]
#define IDXR(A, I, J, R, C) A[(I)*(C)+(J)]

//==========================================================================
// FLOP Counters for different matrix operations
//==========================================================================

//gets the item in a flat matrix given the indices and col/row major order
#define GEMM_FLOP(N, M, P) \
  (  ((2.0*(double)P-1.0)*(double)N*(double)M) + \
     (2.0*(double)N*(double)M) + \
     ((double)N*(double)P)   )

//TODO: fix these! you forgot to include scalar, and you messed up the division
#define TRSM_FLOP_DIAG(M, P) \
  ((double)((double)M*(double)P*(5.0 + 3.0/2.0*((double)M-1))))
#define TRSM_FLOP_UNIT(M, P) \
  ((double)((double)M*(double)P*((double)M + 2.0)))

//https://www.spec.org/workshops/2006/papers/08_luszczek-hpcc.pdf
#define LU_SOLVE_FLOP(N) \
  (2.0/3.0*(double)N*(double)N*(double)N + 3.0/2.0*(double)N*(double)N)

#define STREAM_COPY_FLOP(size)      0
#define STREAM_COPY_DP_BYTES(size)  (2*size*sizeof(double))
#define STREAM_SCALE_FLOP(size)     (size)
#define STREAM_SCALE_DP_BYTES(size) (2*size*sizeof(double))
#define STREAM_SUM_FLOP(size)       (size)
#define STREAM_SUM_DP_BYTES(size)   (3*size*sizeof(double))
#define STREAM_TRIAD_FLOP(size)     (2*size)
#define STREAM_TRIAD_DP_BYTES(size) (3*size*sizeof(double))

//http://www.fftw.org/speed/method.html
#define FFT_FLOP(size) (5*size*(log(size)/log(2)))

#define RANDOM_ACCESS_GUP(size) (4.0*((double)(size))/1e9)


#define PTRANS_FLOP(size) ((double)(size)*(double)(size))

//==========================================================================
// BLOCK SIZES FOR BLOCKED MATRIX FUNCTIONS (mostly just for reference)
//
// Typical L1 D$ is around 32kB, so to fit 3 square panels of doubles for 
// A, B, and C, we need 64k/8 = 3x^2. 
// So x = 24 is decent size for doubles. For floats, we can fit 2x as many,
// So x = 48.
//
// NOTE: these numbers don't mattaer much. as long as their in the 50-100
//       range, performance is about the same
//==========================================================================
#define OPTIMAL_BLOCK_D 64
#define OPTIMAL_BLOCK_S 128

#endif // __MATRIX_TYPES_H__
