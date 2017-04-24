//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "gemm/dgemm_la_core.h"
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "transpose/dtranspose_la_core.h"
#include "verify/dgemm.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;
uint64_t SCRATCH_SIZE = 0;
bool USE_SCRATCH = true;
bool USE_SCRATCH_FOR_TRANSPOSE = true;
bool USE_PANEL = true;

//==========================================================================

static const IDX BLOCK_MIN      = 128; //was 2
static const IDX BLOCK_MAX      = 256;
static const IDX BLOCK_SCALE    = 2;

static const double alpha       = 2.5;
static const double beta        = 3.0;

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;

static const bool YES_COL_MAJOR = 1;
static const bool NO_COL_MAJOR  = 0;

static const bool YES_TRANS     = 1;
static const bool NO_TRANS      = 0;

static void la_trans_and_swap(double **A, IDX N, IDX M)
{
  double *A_trans = malloc_matrix_d(N, M);
  dtranspose_la_core(*A, A_trans, N, M);
  free_matrix_d(*A);
  *A = A_trans;
}

//transpose_matrix_in_place_drc(C, N, M);
//transpose_matrix_in_place_drc(A, N, P);
//transpose_matrix_in_place_drc(B, P, M);
static void perform_transposes(double **C, double **A, double **B, 
  int N, int M, int P, bool col_major, bool A_trans, bool B_trans)
{
  if(!col_major) {
    la_trans_and_swap(C, N, M);
    if(A_trans){
      la_trans_and_swap(A, N, P);
    }
    if(!B_trans){
      la_trans_and_swap(B, P, M);
    }
  } else {
    if(!A_trans){
      la_trans_and_swap(A, N, P);
    }
    if(B_trans){
      la_trans_and_swap(B, P, M);
    }
  }
}

//need: C=col-major, A=row-major, B=col-major
static void do_inner_test(double **C, double **A, double **B, 
  int N, int M, int P, int BS, bool col_major, bool A_trans, bool B_trans) 
{
  //transpose C,A,B if needed
  perform_transposes(C, A, B, N, M, P, col_major, A_trans, B_trans);

  //run computation
  for(IDX i=0; i<N; i+=BS) {
    for(IDX j=0; j<M; j+=BS) {
      IDX height = MIN(N-i, BS);
      IDX width = MIN(M-j, BS);
      dgemm_la_core(*C, *A, *B, alpha, beta, 
        N, M, P, i, height, j, width);
    }
  }

  //transpose C,A,B if needed
  perform_transposes(C, A, B, N, M, P, col_major, A_trans, B_trans);
}

static void do_test(IDX N, IDX M, IDX P, IDX BS, bool col_major, 
  bool A_trans, bool B_trans)
{
  double *C, *A, *B, *C_orig;

  if (col_major) {
    C = create_matrix_dc(N, M, FILL);
    A = create_matrix_dc(N, P, FILL);
    B = create_matrix_dc(P, M, FILL);
    C_orig = copy_matrix_dc(C, N, M);
    if(DEBUG){
      dump_matrix_dc("C-before", C, N, M);
      dump_matrix_dc("A-before", A, N, P);
      dump_matrix_dc("B-before", B, P, M);
    }
  } else {
    C = create_matrix_dr(N, M, FILL);
    A = create_matrix_dr(N, P, FILL);
    B = create_matrix_dr(P, M, FILL);
    C_orig = copy_matrix_dr(C, N, M);
    if(DEBUG){
      dump_matrix_dr("C-before", C, N, M);
      dump_matrix_dr("A-before", A, N, P);
      dump_matrix_dr("B-before", B, P, M);
    }
  }

  double start = get_real_time();
  do_inner_test(&C, &A, &B, N, M, P, BS, col_major, A_trans, B_trans);
  double duration = get_real_time() - start;

  //if(DEBUG){
  //  if(col_major){
  //    dump_matrix_dc("C-after", C, N, M);
  //    dump_matrix_dc("A-after", A, N, P);
  //    dump_matrix_dc("B-after", B, P, M);
  //  } else {
  //    dump_matrix_dr("C-after", C, N, M);
  //    dump_matrix_dr("A-after", A, N, P);
  //    dump_matrix_dr("B-after", B, P, M);
  //  }
  //}

  double flop = GEMM_FLOP(N, M, P);
  PRINT("N=%4d M=%4d P=%4d BS=%4d (%s:%s:%s) | took % 10.6f secs | ", 
    N, M, P, BS,
    (col_major ? "col" : "row"), (A_trans ? "Atrns" : "Anorm"), 
    (B_trans ? "Btrns" : "Bnorm"), duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s || ", ((double)((2*P - 1)*N*M)) / duration / 1e6);

  verify_dgemm(C_orig, A, B, N, M, P, alpha, beta, col_major,
    A_trans, B_trans, C);

  free_matrix_d(C);
  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(C_orig);
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
  uint64_t tmp_scratch_size;
  IDX tmp_idx;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(!strcmp(argv[i], "--debug")){
      DEBUG = true;
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
    else if(!strcmp(argv[i], "--no_scratch")){
      USE_SCRATCH = false;
      USE_SCRATCH_FOR_TRANSPOSE = false;
    }
    else if(!strcmp(argv[i], "--no_scratch_for_xpose")){
      USE_SCRATCH_FOR_TRANSPOSE = false;
    }
    else if(!strcmp(argv[i], "--no_panel")){
      USE_PANEL = false;
    }
    else if(sscanf(argv[i], "--scratch_size=%llu", &tmp_scratch_size) == 1){
      if (tmp_scratch_size == 0) {
        printf("invalid log2 scratch_size");
        exit(0);
      }
      SCRATCH_SIZE = (1 << tmp_scratch_size);
      printf("[dgemm]: scratch_size is %llu\n", SCRATCH_SIZE);
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //---------------------------------------------------------------------------
  if (USE_SCRATCH && SCRATCH_SIZE == 0) {
    printf("you must specify scratch_size in bytes\n");
    exit(0);
  }

  //---------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //---------------------------------------------------------------------------

  //set breakpoint here, then make command window huge

  for(IDX NMP=nmp_start; NMP <= nmp_end; NMP *= NMP_SCALE){
    for(IDX BS=bs_start; BS<=bs_end && BS<=nmp_end; BS *= BLOCK_SCALE){
      if(idx == 0){
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS,  NO_TRANS);
      } else if(idx == 1){
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS,  YES_TRANS);
      } else if(idx == 2){
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS, NO_TRANS);
      } else if(idx == 3){
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS, YES_TRANS);
      } else {
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS,  NO_TRANS);
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, NO_TRANS,  YES_TRANS);
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS, NO_TRANS);
        do_test(NMP, NMP, NMP, BS, YES_COL_MAJOR, YES_TRANS, YES_TRANS);
        /*
        do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  NO_TRANS,  NO_TRANS);
        do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  NO_TRANS,  YES_TRANS);
        do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  YES_TRANS, NO_TRANS);
        do_test(NMP, NMP, NMP, BS, NO_COL_MAJOR,  YES_TRANS, YES_TRANS);
        */
      }
    }
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
