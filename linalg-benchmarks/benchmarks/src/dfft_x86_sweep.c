//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// x86 implementation of the HPCC FFT benchmark
// Naive Algorithm pretty much straight from CLRS Algorithms textbook
//=========================================================================

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <fftw3.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;

//==========================================================================

static const IDX VEC_MIN      = (1 << 4);
static const IDX VEC_MAX      = (1 << 16);
static const IDX VEC_SCALE    = 2;

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

void do_fft_with_fftw(IDX size)
{
  fftw_complex *in, *out;
  fftw_plan p;

  //--------------------------- CONFIG -------------------------------
  in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
  for(IDX i=0; i<size; ++i){
    in[i][0] = 0.0;
  }

  if(DEBUG){
    printf("FTTW in=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", in[i][0], in[i][1]);
    }
    printf("]\n");
  }

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  p = fftw_plan_dft_1d(size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p); /* repeat as needed */
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = FFT_FLOP(size);
  printf("SIZE=%8d | fftw took % 10.6f secs | ", size, duration);
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  if(DEBUG){
    printf("FTTW out=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", out[i][0], out[i][1]);
    }
    printf("]\n");
  }

  //--------------------------- CLEANUP ----------------------------------
  fftw_destroy_plan(p);
  fftw_free(in); 
  fftw_free(out);
}

void do_fft(IDX size)
{
  IDX i;
  //--------------------------- CONFIG -------------------------------
  //create the input, and set only real parts to non-zero
  double *A = create_matrix_dr(size*2, 1, FILL);
  for(i=0; i<size; ++i){
    A[i*2+1] = 0.0;
  }

  if(DEBUG){
    printf("GSL in=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", A[i*2], A[i*2+1]);
    }
    printf("]\n");
  }

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  gsl_fft_complex_wavetable * wavetable =
      gsl_fft_complex_wavetable_alloc(size);
  gsl_fft_complex_workspace * workspace =
      gsl_fft_complex_workspace_alloc(size);
  gsl_fft_complex_forward(A, 1, size, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = FFT_FLOP(size);
  printf("SIZE=%8d | gsl took % 10.6f secs | ", size, duration);
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  if(DEBUG){
    printf("GSL out=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", A[i*2], A[i*2+1]);
    }
    printf("]\n");
  }


  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
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

  //---------------------------------------------------------------------------
  //parse args
  int i;
  for(i=1; i<argc; ++i){
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
  IDX SIZE;
  for(SIZE=size_start; SIZE <= size_end; SIZE *= VEC_SCALE){
    do_fft(SIZE);
    do_fft_with_fftw(SIZE);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
