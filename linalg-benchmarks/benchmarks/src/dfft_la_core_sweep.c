//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of the HPCC FFT benchmark
// Naive Algorithm pretty much straight from CLRS Algorithms textbook
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "fft/dfft_la_core.h"
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/dfft.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;
uint64_t SCRATCH_SIZE = 0;
bool USE_SCRATCH = true;

//==========================================================================

static const IDX VEC_MIN      = (1 << 4);
static const IDX VEC_MAX      = (1 << 16);
static const IDX VEC_SCALE    = 2;

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

void do_fft(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  //create the input, and set only real parts to non-zero
  Complex *A = (Complex*) create_matrix_dr(size*2, 1, FILL);
  for(IDX i=0; i<size; ++i){
    A[i].im = 0.0;
  }

  //gsl solves in-place, so make copy
  double *A_orig = copy_matrix_dr((double *)A, size*2, 1);
  double *Y_gsl = copy_matrix_dr((double *)A, size*2, 1);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  dfft_la_core(A, size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = FFT_FLOP(size);
  printf("SIZE=%8d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MFLOP/s || ", flop/(duration*1e6));

  //--------------------------- VERIFY -------------------------------------
  verify_dfft((double *)Y_gsl, (double *)A_orig, (double *)A, size);

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d((double *)A);
  free_matrix_d((double *)A_orig);
  free_matrix_d((double *)Y_gsl);
}

//==========================================================================
// MAIN
//==========================================================================

int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX size_start = VEC_MIN;
  IDX size_end = VEC_MAX;

  unsigned tmp_seed;
  IDX tmp_log_size;
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
    else if(sscanf(argv[i], "--log_size=%u", &tmp_log_size) == 1){
      size_start = 1 << tmp_log_size;
      size_end = 1 << tmp_log_size;
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
      printf("[dfft]: scratch_size is %llu\n", SCRATCH_SIZE);
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //---------------------------------------------------------------------------
  if(!USE_SCRATCH) {
    printf("ERROR: non-scratch implemention not provided right now.\n");
    exit(0);
  }
  if (USE_SCRATCH && SCRATCH_SIZE == 0) {
    printf("you must specify scratch_size in bytes\n");
    exit(0);
  }

  //---------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //---------------------------------------------------------------------------

  //set breakpoint here, then make command window huge
  for(IDX SIZE=size_start; SIZE <= size_end; SIZE *= VEC_SCALE){
    do_fft(SIZE);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
