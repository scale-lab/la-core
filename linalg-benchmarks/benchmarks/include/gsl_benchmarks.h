//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Matrix Utility functions for creating/destroying and manipulating matrices
// in column-order or row-order in memory. Additionally, provides some other
// miscellaneous things such as FLOP equation macros, and matrix indexing
// macros
//==========================================================================

#ifndef __GSL_BENCHMARK_H__
#define __GSL_BENCHMARK_H__

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_utilities.h"

//==========================================================================
// THIS IS JUST FOR REFERENCE
//typedef struct
//{
//  size_t size1;
//  size_t size2;
//  size_t tda;
//  double * data;
//  gsl_block * block;
//  int owner;
//} gsl_matrix;
//
// gsl_matrix_view gsl_matrix_view_array(double * base, size_t n1, size_t n2)
//==========================================================================



//==========================================================================
// Check if gsl and my implementation differ by more than factor of epsilon
//==========================================================================

// what our error threshold is (arbitrary?)
#define VERIFY_EPSILON 1e-4

#define MARGIN_EXCEEDED(num1, num2) \
  (fabs(num1 - num2) > VERIFY_EPSILON * fmin(fabs(num1), fabs(num2)))



//==========================================================================
//VERIFY BLAS DGEMM WITH GSL
//
//int gsl_blas_dgemm(CBLAS_TRANSPOSE_t TransA, CBLAS_TRANSPOSE_t TransB,
//  double alpha, const gsl_matrix * A, const gsl_matrix * B,
//  double beta, gsl_matrix * C);
//
// Cmn = alpha*op(Anp)*op(Bpm) + beta*Cmn
//==========================================================================

void verify_gsl_dgemm(double *C, double *A, double *B,
  double alpha, double beta, bool col_major, bool trans_a, bool trans_b, 
  IDX N, IDX M, IDX P, double *C_solved)
{
  double *C_gsl = (double *)malloc(sizeof(double)*N*M);
  bool errors = false;

  //==========================================================================
  double start = get_real_time();
  if (col_major) {
    transpose_matrix_in_place_dcr(C, N, M);
    transpose_matrix_in_place_dcr(A, N, P);
    transpose_matrix_in_place_dcr(B, P, M);
  }
  gsl_matrix_view gsl_C = gsl_matrix_view_array(C, N, M);
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, N, P);
  gsl_matrix_view gsl_B = gsl_matrix_view_array(B, P, M);
  gsl_blas_dgemm((trans_a ? CblasTrans : CblasNoTrans),
                 (trans_b ? CblasTrans : CblasNoTrans),
                 alpha, &gsl_A.matrix, &gsl_B.matrix,
                 beta, &gsl_C.matrix);
  double duration = get_real_time() - start;
  //==========================================================================

  printf("gsl took %8.6f secs | ", duration);
  printf("% 10.3f MFLOP/s | ", GEMM_FLOP(N, M, P)/duration/1e6);

  for (IDX i = 0; i < N; ++i) {
    for (IDX j = 0; j < M; ++j) {
      double num1 = col_major ? IDXC(C_solved,i,j,N,M) : IDXR(C_solved,i,j,N,M);
      double num2 = IDXR(C_gsl, i, j, N, M);
      if (MARGIN_EXCEEDED(num1, num2)){
        printf("Error: C_solved[%d][%d]=% 10.6f, C_gsl[%d][%d]=% 10.6f\n",
          i, j, num1, i, j, num2);
        errors = true;
        dump_matrix_dr("C_solved", C, N, M);
        dump_matrix_dr("C_gsl", C_gsl, N, M);
        goto END_LOOP;
      }
    }
  }
END_LOOP:
  if (!errors) {
    printf("matrix correct\n");
  }

  free(correct_C);
}

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

void verify_gsl_dtrsm(double *A, double *B, double *B_orig, double *B_solved,
  IDX M, IDX P, bool right, bool upper, bool unit, bool trans, double alpha)
{
  bool errors = false;
  IDX B_rows = right ? P : M;
  IDX B_cols = right ? M : P;

  double start = getRealTime();
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, M, M);
  gsl_matrix_view gsl_B = gsl_matrix_view_array(B, B_rows, B_cols);
  gsl_blas_dtrsm((right ? CblasRight : CblasLeft),
                 (upper ? CblasUpper : CblasLower),
                 (trans ? CblasTrans : CblasNoTrans),
                 (unit ? CblasUnit : CblasNonUnit),
                 alpha, &gsl_A.matrix, &gsl_B.matrix);
  double duration = getRealTime() - start;

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





//==========================================================================
// VERIFY LU FACTORIZATION SOLVE WITH GSL
//
//int gsl_linalg_LU_decomp(gsl_matrix * A, gsl_permutation * p, int * signum);
//
//int gsl_linalg_LU_solve(const gsl_matrix * LU, const gsl_permutation * p,
//                        const gsl_vector * b, gsl_vector * x);
//
//A is row-major, B is col major! since gsl_linalg_LU_solve only takes 1 
//B vector at a time!
//==========================================================================

void verify_gsl_lu_dsolve(double *A_gsl, double *B_gsl, 
  double *A_orig, double *B_orig, double *A_solved, double *B_solved, 
  IDX N, IDX P)
{
  bool errors = false;
  double *X_gsl = create_matrix_dc(N, P, false);

  //========================================================================
  double start = getRealTime();
  gsl_matrix_view A_view = gsl_matrix_view_array(A_gsl, N, N);
  gsl_permutation * p = gsl_permutation_alloc(N);
  int s;
  gsl_linalg_LU_decomp(&A_view.matrix, p, &s);

  for(IDX col=0; col<P; ++col){
    double *B_col = &(IDXC(B_gsl, 0, col, N, P));
    double *X_col = &(IDXC(X_gsl, 0, col, N, P));
    gsl_vector_view B_view = gsl_vector_view_array(B_col, N);
    gsl_vector_view X_view = gsl_vector_view_array(X_col, N);
    gsl_linalg_LU_solve(&A_view.matrix, p, &B_view.vector, &X_view.vector);
  }
  double duration = getRealTime() - start;
  //========================================================================

  double flop = TRSM_FLOP_UNIT(N, P) + TRSM_FLOP_DIAG(N, P) + LU_FLOP(N);
  printf("gsl took %8.6f secs | ", duration);
  printf("% 10.3f MFLOP/s | ", flop / duration / 1e6);

  for (IDX i = 0; i < N; ++i) {
    for (IDX j = 0; j < P; ++j) {
      double num1 = IDXC(B_solved, i, j, N, P);
      double num2 = IDXC(X_gsl, i, j, N, P);
      if (MARGIN_EXCEEDED(num1, num2)) {
        printf("\nError: X_solved[%d][%d]=% 12.6f, X_gsl[%d][%d]=% 12.6f\n",
          i, j, num1, i, j, num2);
        errors = true;
        dump_matrix_dr("A_orig", A_orig, N, N);
        dump_matrix_dr("B_orig", B_orig, N, P);
        dump_matrix_dr("A_solved", A_solved, N, N);
        dump_matrix_dr("B_solved", B_solved, N, P);
        dump_matrix_dr("A_gsl", A_gsl, N, N);
        dump_matrix_dr("B_gsl", B_gsl, N, P);
        dump_matrix_dr("X_gsl", X_gsl, N, P);
        goto END_LOOP;
      }
    }
  }
END_LOOP:
  if (!errors) {
    printf("lu solve correct\n");
  }
}


#endif //__GSL_BENCHMARK_H__