//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Used for verifying the results of a dgemv_spv against the GSL implementation
//==========================================================================

#ifndef __VERIFY_DGEMV_SPV_H__
#define __VERIFY_DGEMV_SPV_H__

#include <stdbool.h>

#include <gsl/gsl_spmatrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_spblas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/common.h"

void verify_dgemv_spv(double *A_data_ptr, uint32_t *A_col_ptr,
  uint32_t *A_row_ptr, double *b, double *x_solved, double *x_gsl, 
  IDX N, IDX M)
{
  bool errors = false;

  //==========================================================================
  gsl_spmatrix *gsl_A = gsl_spmatrix_alloc(N, M);
  IDX i, j, offset_j;
  for (i = 0; i < N; ++i) {
    IDX row_items = (i == 0 ? A_row_ptr[i] : A_row_ptr[i]-A_row_ptr[i-1]);
    IDX row_offset = (i == 0 ? 0 : A_row_ptr[i-1]);
    for (j = 0; j < M; ++j) {
      for (offset_j = row_offset; offset_j<(row_offset+row_items); ++offset_j) {
        if(A_col_ptr[offset_j] == j) {
          gsl_spmatrix_set(gsl_A, i, j, A_data_ptr[offset_j]);
          break;
        }
      }
    }
  }
  //==========================================================================
  double start = get_real_time();
  gsl_vector_view gsl_b = gsl_vector_view_array(b, M);
  gsl_vector_view gsl_x = gsl_vector_view_array(x_gsl, N);
  gsl_spblas_dgemv(CblasNoTrans, 1.0, gsl_A, 
                   &gsl_b.vector, 0.0, &gsl_x.vector);
  double duration = get_real_time() - start;
  //==========================================================================

  PRINT("gsl took %8.6f secs | ", duration);
  PRINT("% 10.3f MFLOP/s | ", GEMM_FLOP(N, M, 1)/duration/1e6);

  for (i = 0; i < N; ++i) {
    double num1 = x_solved[i];
    double num2 = x_gsl[i];
    if (MARGIN_EXCEEDED(num1, num2)){
      PRINT("Error: x_solved[%d]=% 10.6f, x_gsl[%d]=% 10.6f\n", i, num1, i, num2);
      errors = true;
      dump_matrix_dr("x_solved", x_solved, 1, N);
      dump_matrix_dr("x_gsl", x_gsl, 1, N);
      goto END_LOOP;
    }
  }
END_LOOP:
  if (!errors) {
    PRINT("matrix correct\n");
  }
}


#endif //__VERIFY_DGEMV_SPV_H__
