//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "trsm/dtrsm_la_core.h"
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/dtrsm.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;
uint64_t SCRATCH_SIZE = 0;
bool USE_SCRATCH = true;

//==========================================================================

static const IDX BLOCK_MIN      = 128; //was 2
static const IDX BLOCK_MAX      = 256;
static const IDX BLOCK_SCALE    = 2;

static const double alpha       = 2.5;

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;

static const bool YES_COL_MAJOR = 1;
static const bool NO_COL_MAJOR  = 0;

static const bool YES_TRANS     = 1;
static const bool NO_TRANS      = 0;

#define A_RIGHT true
#define A_LEFT false
#define A_UPPER true
#define A_LOWER false
#define A_UNIT true
#define A_NOT_UNIT false
#define A_TRANS true
#define A_NOT_TRANS false

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

//only do lower for now
void do_test(IDX M, IDX P, bool right, bool upper, bool unit, bool trans)
{
  double *A, *B, *B_orig, *gsl_A, *gsl_B, *gsl_B_solved, *gsl_B_orig;
  bool real_upper = trans ? !upper : upper;

  //algorithm requirement #2 ('right' determines row order and indices)
  if (right) {
    A = create_matrix_trianglular_dc(M, M, upper, unit);
    B = create_matrix_dr(P, M, FILL);
    B_orig = copy_matrix_dr(B, P, M);
  } 
  else {
    A = create_matrix_trianglular_dr(M, M, upper, unit);
    B = create_matrix_dc(M, P, FILL);
    B_orig = copy_matrix_dc(B, M, P);
  }

  double start = get_real_time();
  double *A_actual = trans ? transpose_matrix_dcr(A, M, M) : A;
  dtrsm_la_core(A_actual, B, M, P, right, real_upper, unit, alpha);
  if (trans) {
    free_matrix_d(A_actual);
  }
  double duration = get_real_time() - start;

  double flop = unit ? TRSM_FLOP_UNIT(M, P) : TRSM_FLOP_DIAG(M, P);
  printf("M=%4u P=%4u %s%s%s%s | BASE took % 10.6f secs | ", M, P, 
    (right ? "R" : "L"), (upper ? "U" : "L"), 
    (trans ? "T" : "N"), (unit ? "U" : "D"), duration);
  printf("% 9.3f MFLOP | ", flop/1e6);
  printf("% 12.3f MFLOP/s || ", flop/(duration*1e6));

  if (right) {
    gsl_A = transpose_matrix_dcr(A, M, M);
    gsl_B = copy_matrix_dr(B_orig, P, M);
    gsl_B_orig = copy_matrix_dr(B_orig, P, M);
    gsl_B_solved = copy_matrix_dr(B, P, M);
  }
  else {
    gsl_A = copy_matrix_dr(A, M, M);
    gsl_B = transpose_matrix_dcr(B_orig, M, P);
    gsl_B_orig = transpose_matrix_dcr(B_orig, M, P);
    gsl_B_solved = transpose_matrix_dcr(B, M, P);
  }
  verify_dtrsm(gsl_A, gsl_B, gsl_B_orig, gsl_B_solved,
    M, P, right, upper, unit, trans, alpha);

  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(B_orig);
  free_matrix_d(gsl_A);
  free_matrix_d(gsl_B);
  free_matrix_d(gsl_B_orig);
  free_matrix_d(gsl_B_solved);
}

//only do lower for now
void do_test_fast(IDX M, IDX P, bool right, bool upper, bool unit, bool trans)
{
  double *A, *B, *B_orig, *gsl_A, *gsl_B, *gsl_B_solved, *gsl_B_orig;
  bool real_upper = trans ? !upper : upper;

  //algorithm requirement #2 ('right' determines row order and indices)
  if (right) {
    printf("not implemented");
    exit(0);
  } 
  else {
    if(upper){
      printf("not implemented");
      exit(0);
    }
    A = create_matrix_trianglular_dc(M, M, upper, unit);
    B = create_matrix_dc(M, P, FILL);
    B_orig = copy_matrix_dc(B, M, P);
  }

  double start = get_real_time();
  if(trans){
    printf("not implemented");
    exit(0);
  }
  dtrsm_la_core_fast(A, B, M, P, right, real_upper, unit, alpha);
  double duration = get_real_time() - start;

  double flop = unit ? TRSM_FLOP_UNIT(M, P) : TRSM_FLOP_DIAG(M, P);
  printf("M=%4u P=%4u %s%s%s%s | FAST took % 10.6f secs | ", M, P, 
    (right ? "R" : "L"), (upper ? "U" : "L"), 
    (trans ? "T" : "N"), (unit ? "U" : "D"), duration);
  printf("% 9.3f MFLOP | ", flop/1e6);
  printf("% 12.3f MFLOP/s || ", flop/(duration*1e6));

  gsl_A = transpose_matrix_dcr(A, M, M);
  gsl_B = transpose_matrix_dcr(B_orig, M, P);
  gsl_B_orig = transpose_matrix_dcr(B_orig, M, P);
  gsl_B_solved = transpose_matrix_dcr(B, M, P);

  verify_dtrsm(gsl_A, gsl_B, gsl_B_orig, gsl_B_solved,
    M, P, right, upper, unit, trans, alpha);

  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(B_orig);
  free_matrix_d(gsl_A);
  free_matrix_d(gsl_B);
  free_matrix_d(gsl_B_orig);
  free_matrix_d(gsl_B_solved);
}


int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX nmp_start = NMP_MIN;
  IDX nmp_end = NMP_MAX;

  unsigned tmp_seed;
  IDX tmp_nmp;
  uint64_t tmp_scratch_size;

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
    else if(!strcmp(argv[i], "--no_scratch")){
      USE_SCRATCH = false;
    }
    else if(sscanf(argv[i], "--scratch_size=%llu", &tmp_scratch_size) == 1){
      if (tmp_scratch_size == 0) {
        printf("invalid log2 scratch_size");
        exit(0);
      }
      SCRATCH_SIZE = (1 << tmp_scratch_size);
      printf("[dtrsm]: scratch_size is %llu\n", SCRATCH_SIZE);
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
    do_test(NMP, 1, A_RIGHT, A_UPPER, A_UNIT,     A_NOT_TRANS);
    do_test(NMP, 1, A_RIGHT, A_UPPER, A_NOT_UNIT, A_NOT_TRANS);
    do_test(NMP, 1, A_LEFT,  A_LOWER, A_UNIT,     A_NOT_TRANS);
    do_test(NMP, 1, A_LEFT,  A_LOWER, A_NOT_UNIT, A_NOT_TRANS);
    do_test(NMP, 1, A_RIGHT, A_LOWER, A_UNIT,     A_NOT_TRANS);
    do_test(NMP, 1, A_RIGHT, A_LOWER, A_NOT_UNIT, A_NOT_TRANS);
    do_test(NMP, 1, A_LEFT,  A_UPPER, A_UNIT,     A_NOT_TRANS);
    do_test(NMP, 1, A_LEFT,  A_UPPER, A_NOT_UNIT, A_NOT_TRANS);

    do_test(NMP, 1, A_RIGHT, A_UPPER, A_UNIT,     A_TRANS);
    do_test(NMP, 1, A_RIGHT, A_UPPER, A_NOT_UNIT, A_TRANS);
    do_test(NMP, 1, A_LEFT,  A_LOWER, A_UNIT,     A_TRANS);
    do_test(NMP, 1, A_LEFT,  A_LOWER, A_NOT_UNIT, A_TRANS);
    do_test(NMP, 1, A_RIGHT, A_LOWER, A_UNIT,     A_TRANS);
    do_test(NMP, 1, A_RIGHT, A_LOWER, A_NOT_UNIT, A_TRANS);
    do_test(NMP, 1, A_LEFT,  A_UPPER, A_UNIT,     A_TRANS);
    do_test(NMP, 1, A_LEFT,  A_UPPER, A_NOT_UNIT, A_TRANS);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
