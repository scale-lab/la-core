//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of Complex double-precision FFT of
// arbitrary-length vectors
//=========================================================================

#include <stdio.h>
#include <math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "verify/common.h"

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

//V, V_solved are 'packed' in the GSL sense, with n-REAL and n-IMAG alternating
void verify_dfft(double *Y_gsl, double *A, double *Y_solved, IDX size)
{
  double start_verify = get_real_time();
  gsl_fft_complex_wavetable * wavetable = gsl_fft_complex_wavetable_alloc(size);
  gsl_fft_complex_workspace * workspace = gsl_fft_complex_workspace_alloc(size);
  gsl_fft_complex_forward(Y_gsl, 1, size, wavetable, workspace);
  double duration_verify = get_real_time() - start_verify;

  double flop = FFT_FLOP(size);
  printf("gsl took % 10.6f secs | ", duration_verify);
  printf("% 12.3f MFLOP/s || ", flop/(duration_verify*1e6));

  IDX i, j;
  for (i = 0; i < size; i++) {
    if(MARGIN_EXCEEDED(REAL(Y_gsl, i), REAL(Y_solved, i)) ||
       MARGIN_EXCEEDED(IMAG(Y_gsl, i), IMAG(Y_solved, i))) 
    {
      printf("ERROR %d'th element different\n", i);
      printf("A       =[");
      for(j=0; j<size; ++j) {
        printf("{%9.3f,%9.3f},", REAL(A, j), IMAG(A, j));
      }
      printf("]\n\n");
      printf("Y_solved=[");
      for(j=0; j<size; ++j) {
        printf("{%9.3f,%9.3f},", REAL(Y_solved, j), IMAG(Y_solved, j));
      }
      printf("]\n\n");
      printf("Y_gsl   =[");
      for(j=0; j<size; ++j) {
        printf("{%9.3f,%9.3f},", REAL(Y_gsl, j), IMAG(Y_gsl, j));
      }
      printf("]\n\n");
      break;
    }
  }
  printf("result correct\n");

  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);
}
