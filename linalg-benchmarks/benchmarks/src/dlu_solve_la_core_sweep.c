//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "lu_solve/dlu_solve_la_core.h"
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/lu_solve.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;
uint64_t SCRATCH_SIZE = 0;
bool USE_SCRATCH = true;

static const IDX NP_MIN = 4;
static const IDX NP_MAX = 1024;
static const IDX NP_SCALE = 2;

//==========================================================================

static void do_test(IDX N, IDX P)
{
  double *A, *B, *A_orig, *B_orig, *A_gsl, *B_gsl;

  A = create_matrix_dr(N, N, FILL);
  B = create_matrix_dc(N, P, FILL);

  A_orig = copy_matrix_dr(A, N, N);
  B_orig = copy_matrix_dc(B, N, P);

  A_gsl = copy_matrix_dr(A, N, N);
  B_gsl = copy_matrix_dc(B, N, P);  //NOTE: gsl unusually needs col-major

  double start = get_real_time();
  dlu_solve_la_core(A, B, N, P);
  double duration = get_real_time() - start;

  double flop = LU_SOLVE_FLOP(N);
  PRINT("N=%4d P=%4d | SLOW took % 10.6f secs | ", N, P, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s || ", flop/ duration / 1e6);

  verify_lu_solve(A_gsl, B_gsl, A_orig, B_orig, B, N, P);

  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(A_orig);
  free_matrix_d(B_orig);
  free_matrix_d(A_gsl);
  free_matrix_d(B_gsl);
}

static void do_test_fast(IDX N)
{
  IDX P = 1;
  double *A, *B, *A_orig, *B_orig, *A_gsl, *B_gsl;

  A = create_matrix_dr(N, N, FILL);
  B = create_matrix_dc(N, P, FILL);

  A_orig = copy_matrix_dr(A, N, N);
  B_orig = copy_matrix_dc(B, N, P);

  A_gsl = copy_matrix_dr(A, N, N);
  B_gsl = copy_matrix_dc(B, N, P);  //NOTE: gsl unusually needs col-major

  double start = get_real_time();
  dlu_solve_la_core_fast(A, B, N);
  double duration = get_real_time() - start;

  double flop = LU_SOLVE_FLOP(N);
  PRINT("N=%4d P=%4d | FAST took % 10.6f secs | ", N, P, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s || ", flop/ duration / 1e6);

  verify_lu_solve(A_gsl, B_gsl, A_orig, B_orig, B, N, P);

  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(A_orig);
  free_matrix_d(B_orig);
  free_matrix_d(A_gsl);
  free_matrix_d(B_gsl);
}

int main(int argc, char **argv)
{
  //-------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX np_start  = NP_MIN;
  IDX np_end    = NP_MAX;

  unsigned tmp_seed;
  IDX tmp_log_size;
  uint64_t tmp_scratch_size;

  //-------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(!strcmp(argv[i], "--debug")){
      DEBUG = true;
    }
    else if(sscanf(argv[i], "--log_size=%u", &tmp_log_size) == 1){
      np_start = (1 << tmp_log_size);
      np_end = (1 << tmp_log_size);
    }
    else if(!strcmp(argv[i], "--no_scratch")){
      USE_SCRATCH = false;
    }
    else if(sscanf(argv[i], "--scratch_size=%llu", &tmp_scratch_size) == 1){
      if (tmp_scratch_size == 0) {
        printf("invalid log2 scratch_size");
        exit(0);
      }
      SCRATCH_SIZE = (1 << tmp_scratch_size);
      printf("[lu_solve]: scratch_size is %llu\n", SCRATCH_SIZE);
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //-------------------------------------------------------------------------
  if (USE_SCRATCH && SCRATCH_SIZE == 0) {
    printf("you must specify scratch_size in bytes\n");
    exit(0);
  }

  //-------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //-------------------------------------------------------------------------

  //set breakpoint here, then make command window huge
  for(IDX NP=np_start; NP <= np_end; NP *= NP_SCALE){
    do_test(NP, 1);
    do_test_fast(NP);
  }

  //set breakpoint here, then analyze results
  return 0;
}
