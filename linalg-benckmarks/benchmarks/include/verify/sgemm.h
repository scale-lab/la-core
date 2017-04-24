//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Used for verifying the results of a dgemm against the GSL implementation
//==========================================================================

#ifndef __VERIFY_SGEMM_H__
#define __VERIFY_SGEMM_H__

#include <stdbool.h>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_matrix_float.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/common.h"

//==========================================================================
// THIS IS JUST FOR REFERENCE
//typedef struct
//{
//  size_t size1;
//  size_t size2;
//  size_t tda;
//  float * data;
//  gsl_block * block;
//  int owner;
//} gsl_matrix;
//
// gsl_matrix_view gsl_matrix_view_array(float * base, size_t n1, size_t n2)
//==========================================================================


//==========================================================================
//VERIFY BLAS DGEMM WITH GSL
//
//int gsl_blas_sgemm(CBLAS_TRANSPOSE_t TransA, CBLAS_TRANSPOSE_t TransB,
//  float alpha, const gsl_matrix * A, const gsl_matrix * B,
//  float beta, gsl_matrix * C);
//
// Cmn = alpha*op(Anp)*op(Bpm) + beta*Cmn
//==========================================================================

void verify_sgemm(float *C, float *A, float *B, IDX N, IDX M, IDX P,
  float alpha, float beta, bool col_major, bool trans_a, bool trans_b, 
  float *C_solved)
{
  bool errors = false;

  //don't time this
  if (col_major) {
    transpose_matrix_in_place_scr(C_solved, N, M);
  }

  //double * C_conv = matrix_s_to_d(C, N, M);
  //double * A_conv = matrix_s_to_d(A, N, P);
  //double * B_conv = matrix_s_to_d(B, P, M);

  //==========================================================================
  double start = get_real_time();
  if (col_major) {
    transpose_matrix_in_place_scr(C, N, M);
    transpose_matrix_in_place_scr(A, N, P);
    transpose_matrix_in_place_scr(B, P, M);
  }
  gsl_matrix_float_view gsl_C = gsl_matrix_float_view_array (C, N, M);
  gsl_matrix_float_view gsl_A = gsl_matrix_float_view_array (A, N, P);
  gsl_matrix_float_view gsl_B = gsl_matrix_float_view_array (B, P, M);
  gsl_blas_sgemm((trans_a ? CblasTrans : CblasNoTrans),
                 (trans_b ? CblasTrans : CblasNoTrans),
                 alpha, &gsl_A.matrix, &gsl_B.matrix,
                 beta, &gsl_C.matrix);
  double duration = get_real_time() - start;
  //==========================================================================

  PRINT("gsl took %8.6f secs | ", duration);
  PRINT("% 10.3f MFLOP/s | ", GEMM_FLOP(N, M, P)/duration/1e6);

  for (IDX i = 0; i < N; ++i) {
    for (IDX j = 0; j < M; ++j) {
      float num1 = IDXR(C_solved,i,j,N,M);
      float num2 = IDXR(C, i, j, N, M);
      if (MARGIN_EXCEEDED(num1, num2)){
        PRINT("Error: C_solved[%d][%d]=% 10.6f, C_gsl[%d][%d]=% 10.6f\n",
          i, j, num1, i, j, num2);
        errors = true;
        dump_matrix_sr("C_solved", C_solved, N, M);
        dump_matrix_sr("C_gsl", C, N, M);
        goto END_LOOP;
      }
    }
  }
END_LOOP:
  if (!errors) {
    PRINT("matrix correct\n");
  }

  //free_matrix_d(C_conv);
  //free_matrix_d(A_conv);
  //free_matrix_d(B_conv);
}


#endif //__VERIFY_SGEMM_H__
