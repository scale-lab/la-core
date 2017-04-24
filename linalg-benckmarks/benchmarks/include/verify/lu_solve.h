//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Used for verifying the results of a lu_solve against the GSL implementation
//==========================================================================

#ifndef __VERIFY_LU_SOLVE_H__
#define __VERIFY_LU_SOLVE_H__

#include <stdbool.h>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/common.h"

//==========================================================================
//VERIFY BLAS DGEMM WITH GSL
//
//int gsl_blas_dgemm(CBLAS_TRANSPOSE_t TransA, CBLAS_TRANSPOSE_t TransB,
//  double alpha, const gsl_matrix * A, const gsl_matrix * B,
//  double beta, gsl_matrix * C);
//
// Cmn = alpha*op(Anp)*op(Bpm) + beta*Cmn
//==========================================================================

//NOTE: B is still in col-major!
void verify_lu_solve(double *A, double *B,
  double * A_orig, double *B_orig, double *B_solved, IDX N, IDX P)
{
  bool errors = false;

  //==========================================================================
  double start = get_real_time();
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, N, N);
  int s;
  gsl_permutation * p = gsl_permutation_alloc(N);
  gsl_linalg_LU_decomp (&gsl_A.matrix, p, &s);

  for(IDX i=0; i<P; ++i) {
    gsl_vector_view gsl_B = gsl_vector_view_array (B+(i*N), N);
    gsl_vector *x = gsl_vector_alloc(N);
    gsl_linalg_LU_solve (&gsl_A.matrix, p, &gsl_B.vector, x);
    memcpy(B+(i*N), x->data, sizeof(double)*N);
    gsl_vector_free(x);
  }
  gsl_permutation_free(p);
  double duration = get_real_time() - start;
  //==========================================================================

  PRINT("gsl took %8.6f secs | ", duration);
  PRINT("% 10.3f MFLOP/s | ", LU_SOLVE_FLOP(N)/duration/1e6);

  for (IDX i = 0; i < N; ++i) {
    for (IDX j = 0; j < P; ++j) {
      double num1 = IDXC(B_solved, i, j, N, P);
      double num2 = IDXC(B,        i, j, N, P);
      if (MARGIN_EXCEEDED(num1, num2)){
        PRINT("Error: B_solved[%d][%d]=% 10.6f, B_gsl[%d][%d]=% 10.6f\n",
          i, j, num1, i, j, num2);
        errors = true;
        dump_matrix_dr("A_orig", A_orig, N, N);
        dump_matrix_dc("B_orig", B_orig, N, P);
        dump_matrix_dc("B_solved", B_solved, N, P);
        dump_matrix_dc("B_gsl", B, N, P);
        goto END_LOOP;
      }
    }
  }
END_LOOP:
  if (!errors) {
    PRINT("matrix correct\n");
  }
}



#endif //__VERIFY_LU_SOLVE_H__
