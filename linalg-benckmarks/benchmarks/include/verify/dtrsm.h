//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Used for verifying the results of a dgemm against the GSL implementation
//==========================================================================

#ifndef __VERIFY_DTRSM_H__
#define __VERIFY_DTRSM_H__

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
//VERIFY BLAS-3 TRSM WITH GSL
//
//void cblas_dtrsm(const enum CBLAS_ORDER Order, const enum CBLAS_SIDE Side,
//  const enum CBLAS_UPLO Uplo, const enum CBLAS_TRANSPOSE TransA,
//  const enum CBLAS_DIAG Diag, const int M, const int N,
//  const double alpha, const double *A, const int lda,
//  double *B, const int ldb);
//
// enum CBLAS_ORDER {CblasRowMajor=101, CblasColMajor=102};
// enum CBLAS_TRANSPOSE { CblasNoTrans = 111, CblasTrans = 112, 
//                        CblasConjTrans = 113 };
// enum CBLAS_UPLO { CblasUpper = 121, CblasLower = 122 };
// enum CBLAS_DIAG { CblasNonUnit = 131, CblasUnit = 132 };
// enum CBLAS_SIDE { CblasLeft = 141, CblasRight = 142 };
//==========================================================================

#define TRSM_RIGHT true
#define TRSM_LEFT false
#define TRSM_UPPER true
#define TRSM_LOWER false
#define TRSM_UNIT true
#define TRSM_NOT_UNIT false
#define TRSM_TRANS true
#define TRSM_NOT_TRANS false

void verify_dtrsm(double *A, double *B, double *B_orig, double *B_solved,
  IDX M, IDX P, bool right, bool upper, bool unit, bool trans, double alpha)
{
  bool errors = false;
  IDX B_rows = right ? P : M;
  IDX B_cols = right ? M : P;

  double start = get_real_time();
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, M, M);
  gsl_matrix_view gsl_B = gsl_matrix_view_array(B, B_rows, B_cols);
  gsl_blas_dtrsm((right ? CblasRight : CblasLeft),
                 (upper ? CblasUpper : CblasLower),
                 (trans ? CblasTrans : CblasNoTrans),
                 (unit ? CblasUnit : CblasNonUnit),
                 alpha, &gsl_A.matrix, &gsl_B.matrix);
  double duration = get_real_time() - start;

  double flop = unit ? TRSM_FLOP_UNIT(M, P) : TRSM_FLOP_DIAG(M, P);
  printf("gsl took %8.6f secs | ", duration);
  printf("% 10.3f MFLOP/s | ", flop / duration / 1e6);

  for (IDX i = 0; i < B_rows; ++i) {
    for (IDX j = 0; j < B_cols; ++j) {
      double num1 = IDXR(B_solved, i, j, B_rows, B_cols);
      double num2 = IDXR(B, i, j, B_rows, B_cols);
      if (MARGIN_EXCEEDED(num1, num2)) {
        printf("Error: B_orig[%d][%d]=% 10.6f, correct_B[%d][%d]=% 10.6f\n",
          i, j, num1, i, j, num2);
        errors = true;
        printf("alpha = % 10.6f\n", alpha);
        dump_matrix_dr("A", A, M, M);
        dump_matrix_dr("B_orig", B_orig, B_rows, B_cols);
        dump_matrix_dr("X_solved", B_solved, B_rows, B_cols);
        dump_matrix_dr("correct_X", B, B_rows, B_cols);
        goto END_LOOP;
      }
    }
  }
END_LOOP:
  if (!errors) {
    printf("matrix correct\n");
  }
}

#endif //__VERIFY_DTRSM_H__
