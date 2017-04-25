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

static double alpha = 2.5;
static double beta  = 3.0;

static IDX NMP_MIN = 16;
static IDX NMP_MAX = 1024;
static IDX NMP_SCALE = 2;

static bool YES_COL_MAJOR = 1;
static bool NO_COL_MAJOR = 0;

static bool YES_TRANS = 1;
static bool NO_TRANS = 0;


void do_inner_test(double *C, double *A, double *B, int N, int M, int P, 
  int BS, bool col_major, bool A_trans, bool B_trans) 
{
  for(IDX i=0; i<N; i+=BS) {
    for(IDX j=0; j<M; j+=BS) {
      IDX height = MIN(N-i, BS);
      IDX width = MIN(M-j, BS);
      dgemm_la_core(C, A, B, alpha, beta, N, M, P, i, height, j, width);
    }
  }
}

void do_test(IDX N, IDX M, IDX P, IDX BS, bool col_major, 
  bool A_trans, bool B_trans)
{
  double *C, *A, *B, *gsl_C *gsl_A, *gsl_B,;

  if (col_major) {
    C = create_matrix_dc(N, M, NO_FILL);
    A = create_matrix_dc(N, P, FILL);
    B = create_matrix_dc(P, M, FILL);
  }
  else {
    C = create_matrix_dr(N, M, NO_FILL);
    A = create_matrix_dr(N, P, FILL);
    B = create_matrix_dr(P, M, FILL);
  }

  double start = getRealTime();
  dgemm_la_core(A, B, C, N, M, P, BS, col_major, A_trans, B_trans);
  double duration = getRealTime() - start;

  printf("N=%4d M=%4d P=%4d BS=%4d (%s:%s) | took % 10.6f secs | ", N, M, P, 
    BS, (col_major ? "col" : "row"), (A_trans ? "Atrns" : "Anorm"), 
    (B_trans ? "Btrns" : "Bnorm"), duration);
  printf("% 8.3f MFLOP | ", ((double)((2*P - 1)*N*M)) / 1e6);
  printf("% 8.3f MFLOP/s || ", ((double)((2*P - 1)*N*M)) / duration / 1e6);

  if (col_major) {
    gsl_C = transpose_matrix_dcr(C, N, M);
    gsl_A = transpose_matrix_dcr(A, N, P);
    gsl_B = transpose_matrix_dcr(B, P, M);
  }
  else {
    gsl_C = copy_matrix_dr(C, N, M);
    gsl_A = copy_matrix_dr(A, N, P);
    gsl_B = copy_matrix_dr(B, P, M);
  }
  verify_gsl_dgemm(gsl_C, gsl_A, gsl_B, N, M, P, alpha, beta, A_trans, B_trans);

  destroy_matrix_d(C);
  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(gsl_C);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
}

int main(int argc, char **argv)
{
  srand((unsigned)time(NULL));

  //set breakpoint here, then make command window huge

  for(IDX NMP=NMP_MIN; NMP <= NMP_MAX; NMP *= NMP_SCALE){
    for(IDX BS=BLOCK_MIN; BS<=BLOCK_MAX; BS *= BLOCK_SCALE){
      do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS,  NO_TRANS);
      do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS,  YES_TRANS);
      do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS, NO_TRANS);
      do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS, YES_TRANS);
      do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  NO_TRANS,  NO_TRANS);
      do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  NO_TRANS,  YES_TRANS);
      do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  YES_TRANS, NO_TRANS);
      do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  YES_TRANS, YES_TRANS);
    }
    printf("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}