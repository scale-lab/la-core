//=========================================================================
//naive dgemm c file
//
//Results:
// gsl runs at about ~3.5 GFLOP/s on my machine regardless of input sizes
// my worst results are ~15 MFLOP/s with block-size of 1
// my best results are:
//   ~340 MFLOP/s: block-size of 8, if(min_k == 0) inside inner loop 
//   ~420 MFLOP/s: block-size of 16, if(min_k == 0) moved outside inner loop,
//                 N=M=P=256, matrix A transposed inside dgemm, and 
//                 C,A,B all in column major order as inputs
// 
// block-size larger than 8 had NO EFFECT on performance
//=========================================================================

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "get_real_time.h"
#include "gsl_result_check.h"
#include "matrix_utilities.h"

#define BLOCK_MIN 1
#define BLOCK_MAX 64
#define BLOCK_SCALE 2

/*
#define N_MIN 16
#define N_MAX 1024
#define N_SCALE 4

#define M_MIN 16
#define M_MAX 1024
#define M_SCALE 4

#define P_MIN 16
#define P_MAX 1024
#define P_SCALE 4
*/
#define NMP_MIN 16
#define NMP_MAX 1024
#define NMP_SCALE 2

#define YES_COL_MAJOR 1
#define NO_COL_MAJOR 0

#define YES_TRANS 1
#define NO_TRANS 0

//if col_major: C=col-major, A=row-major, B=col-major order
//else        : C=row-major, A=col-major, B=row-major order
// C[n][m] = A[n][p] * B[p][m]
void dgemm_block(double *A, double *B, double *C, int N, int M, int P,
  int BS, int i, int j, int k, bool col_major, bool trans)
{
  int min_i = i;
  int min_j = j;
  int min_k = k;
  int max_i = (i + BS > N) ? N : (i + BS);  //N!
  int max_j = (j + BS > M) ? M : (j + BS);  //M!
  int max_k = (k + BS > P) ? P : (k + BS);

  //zero result array
  if (min_k == 0 && col_major) {
    for (j = min_j; j < max_j; ++j) {
      for (i = min_i; i < max_i; ++i) {
        IDXC(C, i, j, N, M) = 0;
      }
    }
  }
  if (min_k == 0 && !col_major) {
    for (i = min_i; i < max_i; ++i) {
      for (j = min_j; j < max_j; ++j) {
        IDXR(C, i, j, N, M) = 0;
      }
    }
  }

  //compute result
  if(col_major && trans){
    for(j=min_j; j<max_j; ++j){
      for(i=min_i; i<max_i; ++i){
        for(k=min_k; k<max_k; ++k){
          IDXC(C, i, j, N, M) += IDXR(A, i, k, N, P)*IDXC(B, k, j, P, M);
        }
      }
    }
  }
  else if (col_major && !trans) {
    for (j = min_j; j<max_j; ++j) {
      for (i = min_i; i<max_i; ++i) {
        for (k = min_k; k<max_k; ++k) {
          IDXC(C, i, j, N, M) += IDXC(A, i, k, N, P)*IDXC(B, k, j, P, M);
        }
      }
    }
  }
  else if(!col_major && trans) {
    for (i = min_i; i<max_i; ++i) {
      for (j = min_j; j<max_j; ++j) {
        for (k = min_k; k<max_k; ++k) {
          IDXR(C, i, j, N, M) += IDXC(A, i, k, N, P)*IDXR(B, k, j, P, M);
        }
      }
    }
  }
  else {
    for (i = min_i; i<max_i; ++i) {
      for (j = min_j; j<max_j; ++j) {
        for (k = min_k; k<max_k; ++k) {
          IDXR(C, i, j, N, M) += IDXR(A, i, k, N, P)*IDXR(B, k, j, P, M);
        }
      }
    }
  }
}

// C[n][m] = A[n][p] * B[p][m]
void dgemm(double *A, double *B, double *C, int N, int M, int P, int BS,
  bool col_major, bool trans)
{
  double *A_trans, *A_pass;
  if (col_major && trans) {
    A_trans = transpose_matrix_dcr(A, N, P);
    A_pass = A_trans;
  } 
  else if (!col_major && trans){
    A_trans = transpose_matrix_drc(A, N, P);
    A_pass = A_trans;
  }
  else {
    A_pass = A;
  }

  if(col_major){
    for (int j=0; j<M; j += BS) {
      for (int i=0; i<N; i += BS) {
        for (int k=0; k<P; k += BS) {
          dgemm_block(A_pass, B, C, N, M, P, BS, i, j, k, col_major, trans);
        }
      }
    }
  }
  else {
    for (int i = 0; i<N; i += BS) {
      for (int j = 0; j<M; j += BS) {
        for (int k = 0; k<P; k += BS) {
          dgemm_block(A_pass, B, C, N, M, P, BS, i, j, k, col_major, trans);
        }
      }
    }
  }

  if (trans) {
    destroy_matrix_d(A_trans);
  }
}

void do_test(int N, int M, int P, int BS, bool col_major, bool trans)
{
  double *A, *B, *C, *gsl_A, *gsl_B, *gsl_C;

  if (col_major) {
    A = create_matrix_dc(N, P, FILL);
    B = create_matrix_dc(P, M, FILL);
    C = create_matrix_dc(N, M, NO_FILL);
  }
  else {
    A = create_matrix_dr(N, P, FILL);
    B = create_matrix_dr(P, M, FILL);
    C = create_matrix_dr(N, M, NO_FILL);
  }

  double start = getRealTime();
  dgemm(A, B, C, N, M, P, BS, col_major, trans);
  double duration = getRealTime() - start;

  printf("N=%4d M=%4d P=%4d BS=%4d (%s:%s) | took % 10.6f secs | ", N, M, P, 
    BS, (col_major ? "col" : "row"), (trans ? "trns" : "norm"), duration);
  printf("% 8.3f MFLOP | ", ((double)((2*P - 1)*N*M)) / 1e6);
  printf("% 8.3f MFLOP/s || ", ((double)((2*P - 1)*N*M)) / duration / 1e6);

  if (col_major) {
    gsl_A = transpose_matrix_dcr(A, N, P);
    gsl_B = transpose_matrix_dcr(B, P, M);
    gsl_C = transpose_matrix_dcr(C, N, M);
  }
  else {
    gsl_A = copy_matrix_dr(A, N, P);
    gsl_B = copy_matrix_dr(B, P, M);
    gsl_C = copy_matrix_dr(C, N, M);
  }
  verify_gsl_dgemm(gsl_A, gsl_B, gsl_C, N, M, P);

  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(C);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
  destroy_matrix_d(gsl_C);
}

//NOTE: I confirmed by walking back down the block size that the performane
//on the way up and down for each block size was the same. This was to make
//sure the caches weren't getting 'warmed up' over each iteration. It was 
//verified that no 'warming up' was happening. so i just commented those 
//lines out to save runtime
int main(int argc, char **argv)
{
  srand((unsigned)time(NULL));

  //set breakpoint here, then make command window huge
  /*
  for(int N=N_MIN; N<=N_MAX; N *= N_SCALE){
    for(int M=M_MIN; M<=M_MAX; M*= M_SCALE){
      for(int P=P_MIN; P<=P_MAX; P *= P_SCALE){
        for(int BS=BLOCK_MIN; BS<=BLOCK_MAX; BS *= BLOCK_SCALE){
          do_test(N, M, P, BS, YES_COL_MAJOR, NO_TRANS);
          do_test(N, M, P, BS, YES_COL_MAJOR, YES_TRANS);
          do_test(N, M, P, BS, NO_COL_MAJOR, NO_TRANS);
          do_test(N, M, P, BS, NO_COL_MAJOR, YES_TRANS);
        //for (int BS=BLOCK_MAX/BLOCK_SCALE; BS>=BLOCK_MIN; BS/=BLOCK_SCALE) {
        //  do_test(N, M, P, BS, YES_COL_MAJOR);
        //  do_test(N, M, P, BS, NO_COL_MAJOR);
        //}
        printf("\n");
      }
    }
  }
  */
  for(int NMP=NMP_MIN; NMP <= NMP_MAX; NMP *= NMP_SCALE){
    for(int BS=BLOCK_MIN; BS<=BLOCK_MAX; BS *= BLOCK_SCALE){
      do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS);
      do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS);
      do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR, NO_TRANS);
      do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR, YES_TRANS);
    }
    printf("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}