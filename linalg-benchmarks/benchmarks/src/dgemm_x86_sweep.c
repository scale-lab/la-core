//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <Eigen/Dense>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

//==========================================================================

static const IDX BLOCK_MIN      = 128; //was 2
static const IDX BLOCK_MAX      = 256;
static const IDX BLOCK_SCALE    = 2;

static const double alpha       = 2.5;
static const double beta        = 3.0;

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;

static const bool YES_TRANS     = 1;
static const bool NO_TRANS      = 0;

//==========================================================================

static void do_test_eigen(IDX N, IDX M, IDX P, IDX BS, bool A_trans, 
  bool B_trans)
{
  using namespace Eigen;
  using namespace std;

  MatrixXd A = MatrixXd::Random(N, M);
  MatrixXd B = MatrixXd::Random(N, P);
  MatrixXd C = MatrixXd::Random(P, M);

  double start = get_real_time();
  C = alpha*(A_trans?A.transpose():A)*(B_trans?B.transpose():B) + beta*C;
  double duration = get_real_time() - start;

  double flop = GEMM_FLOP(N, M, P);
  PRINT("N=%4d M=%4d P=%4d BS=%4d (%s:%s) | Eigen took % 10.6f secs | ", 
    N, M, P, BS,
    (A_trans ? "Atrns" : "Anorm"), (B_trans ? "Btrns" : "Bnorm"), duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration / 1e6);

}

static void do_test(IDX N, IDX M, IDX P, IDX BS, bool A_trans, bool B_trans)
{
  double *C, *A, *B;
  C = create_matrix_dr(N, M, FILL);
  A = create_matrix_dr(N, P, FILL);
  B = create_matrix_dr(P, M, FILL);

  double start = get_real_time();
  gsl_matrix_view gsl_C = gsl_matrix_view_array(C, N, M);
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, N, P);
  gsl_matrix_view gsl_B = gsl_matrix_view_array(B, P, M);
  gsl_blas_dgemm((A_trans ? CblasTrans : CblasNoTrans),
                 (B_trans ? CblasTrans : CblasNoTrans),
                 alpha, &gsl_A.matrix, &gsl_B.matrix,
                 beta, &gsl_C.matrix);
  double duration = get_real_time() - start;

  double flop = GEMM_FLOP(N, M, P);
  PRINT("N=%4d M=%4d P=%4d BS=%4d (%s:%s) | GSL   took % 10.6f secs | ", 
    N, M, P, BS,
    (A_trans ? "Atrns" : "Anorm"), (B_trans ? "Btrns" : "Bnorm"), duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration / 1e6);

  free_matrix_d(C);
  free_matrix_d(A);
  free_matrix_d(B);
}

int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX nmp_start = NMP_MIN;
  IDX nmp_end = NMP_MAX;
  IDX bs_start = BLOCK_MIN;
  IDX bs_end = BLOCK_MAX;
  IDX idx = -1;

  unsigned tmp_seed;
  IDX tmp_nmp;
  IDX tmp_bs;
  IDX tmp_idx;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(sscanf(argv[i], "--size=%u", &tmp_nmp) == 1){
      nmp_start = tmp_nmp;
      nmp_end = tmp_nmp;
    }
    else if(sscanf(argv[i], "--bs=%u", &tmp_bs) == 1){
      bs_start = tmp_bs;
      bs_end = tmp_bs;
    }
    else if(sscanf(argv[i], "--idx=%d", &tmp_idx) == 1){
      if(tmp_idx < 0 || tmp_idx > 3) {
        printf("invalid idx, must be [0,3]\n");
        exit(0);
      }
      idx = tmp_idx;
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //---------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //---------------------------------------------------------------------------

  //set breakpoint here, then make command window huge

  for(IDX NMP=nmp_start; NMP <= nmp_end; NMP *= NMP_SCALE){
    for(IDX BS=bs_start; BS<=bs_end && BS<=nmp_end; BS *= BLOCK_SCALE){
      if(idx == 0){
        do_test(NMP, NMP, NMP, BS, NO_TRANS,  NO_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, NO_TRANS,  NO_TRANS);
      } else if(idx == 1){
        do_test(NMP, NMP, NMP, BS, NO_TRANS,  YES_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, NO_TRANS,  YES_TRANS);
      } else if(idx == 2){
        do_test(NMP, NMP, NMP, BS, YES_TRANS, NO_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, YES_TRANS, NO_TRANS);
      } else if(idx == 3){
        do_test(NMP, NMP, NMP, BS, YES_TRANS, YES_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, YES_TRANS, YES_TRANS);
      } else {
        do_test(NMP, NMP, NMP, BS, NO_TRANS,  NO_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, NO_TRANS,  NO_TRANS);
        
        do_test(NMP, NMP, NMP, BS, NO_TRANS,  YES_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, NO_TRANS,  YES_TRANS);

        do_test(NMP, NMP, NMP, BS, YES_TRANS, NO_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, YES_TRANS, NO_TRANS);

        do_test(NMP, NMP, NMP, BS, YES_TRANS, YES_TRANS);
        do_test_eigen(NMP, NMP, NMP, BS, YES_TRANS, YES_TRANS);
      }
    }
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
