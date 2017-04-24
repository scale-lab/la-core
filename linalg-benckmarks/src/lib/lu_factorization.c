
#include "matrix_utilities.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//intel AVX requires 16 byte alignment
#define ALIGNMENT 64

//==========================================================================
// dump_matrix (double-precision)
//==========================================================================

void dump_matrix_dc(const char *name, double *A, IDX h, IDX w)
{
  printf("%s (col-major)\n", name);
  printf("[\n");
  for (IDX i = 0; i < h; ++i) {
    printf("  ");
    for (IDX j = 0; j < w; ++j) {
      if (j != 0) {
        printf(", ");
      }
      printf("%10.6f", IDXC(A, i, j, h, w));
    }
    printf("\n");
  }
  printf("]\n");
}

void dump_matrix_dr(const char *name, double *A, IDX h, IDX w)
{
  printf("%s (row-major):\n", name);
  printf("[\n");
  for (IDX i = 0; i < h; ++i) {
    printf("  ");
    for (IDX j = 0; j < w; ++j) {
      if (j != 0) {
        printf(", ");
      }
      printf("%10.6f", IDXR(A, i, j, h, w));
    }
    printf("\n");
  }
  printf("]\n");
}

//==========================================================================
// create_matrix (double-precision)
//==========================================================================

//col major order
double * create_matrix_dc(IDX h, IDX w, bool fill)
{
  double *A = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  if (fill) {
    for (IDX j = 0; j < w; ++j) {
      for (IDX i = 0; i<h; ++i) {
        IDXC(A, i, j, h, w) = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return A;
}

//row major order
double * create_matrix_dr(IDX h, IDX w, bool fill)
{
  double *A = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  if (fill) {
    for (IDX i = 0; i<h; ++i) {
      for (IDX j = 0; j < w; ++j) {
        IDXR(A, i, j, h, w) = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return A;
}

//==========================================================================
// create_matrix_triangular (double-precision)
//==========================================================================

#define UPPER 1
#define LOWER 0
#define UNIT 1
#define NON_UNIT 0

double * create_matrix_trianglular_dc(IDX h, IDX w, bool upper, bool unit)
{
  double *A = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  for (IDX j = 0; j < w; ++j) {
    for (IDX i = 0; i < h; ++i) {
      if ((j < i && upper) || (j > i && !upper)) {
        IDXC(A, i, j, h, w) = 0.0;
      }
      else if (j == i && unit) {
        IDXC(A, i, j, h, w) = 1.0;
      }
      else {
        IDXC(A, i, j, h, w) = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return A;
}

double * create_matrix_trianglular_dr(IDX h, IDX w, bool upper, bool unit)
{
  double *A = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  for (IDX i = 0; i < h; ++i) {
    for (IDX j = 0; j < w; ++j) {
      if ((j < i && upper) || (j > i && !upper)) {
        IDXR(A, i, j, h, w) = 0.0;
      }
      else if (j == i && unit) {
        IDXR(A, i, j, h, w) = 1.0;
      }
      else {
        IDXR(A, i, j, h, w) = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return A;
}

//==========================================================================
// copy_matrix (double-precision)
//==========================================================================

// col major order
double * copy_matrix_dc(double *A, IDX h, IDX w)
{
  double *B = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  for (IDX i = 0; i < h; ++i) {
    for (IDX j = 0; j < w; ++j) {
      IDXC(B, i, j, h, w) = IDXC(A, i, j, h, w);
    }
  }
  return B;
}


// row major order
double * copy_matrix_dr(double *A, IDX h, IDX w)
{
  double *B = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  for (IDX i = 0; i < h; ++i) {
    for (IDX j = 0; j < w; ++j) {
      IDXR(B, i, j, h, w) = IDXR(A, i, j, h, w);
    }
  }
  return B;
}

//==========================================================================
// transpose_matrix (double-precision)
//==========================================================================

//converts col-major to row-major order
double *transpose_matrix_dcr(double *A, IDX h, IDX w)
{
  double *B = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  for (IDX i = 0; i < h; ++i) {
    for (IDX j = 0; j < w; ++j) {
      IDXR(B, i, j, h, w) = IDXC(A, i, j, h, w);
    }
  }
  return B;
}

//row -> col major order
double *transpose_matrix_drc(double *A, IDX h, IDX w)
{
  double *B = (double *)_aligned_malloc(sizeof(double)*h*w, ALIGNMENT);
  for (IDX j = 0; j < w; ++j) {
    for (IDX i = 0; i < h; ++i) {
      IDXC(B, i, j, h, w) = IDXR(A, i, j, h, w);
    }
  }
  return B;
}

//==========================================================================
// destroy_matrix (double-precision)
//==========================================================================

void destroy_matrix_d(double *A)
{
  _aligned_free(A);
}

//==========================================================================
// miscellaneous calls
//==========================================================================

double get_scalar_double()
{
  return ((double)rand() / (double)RAND_MAX);
}