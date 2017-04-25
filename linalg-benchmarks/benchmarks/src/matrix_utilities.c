//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include "matrix_utilities.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "matrix_types.h"

//intel AVX-256 requires 64 byte alignment, so all matrix allocations will be
//aigned to at leas this
static const unsigned ALIGNMENT = 64;

//==========================================================================
// Platform-independent aligned-malloc/free
//==========================================================================

double * malloc_matrix_d(IDX height, IDX width) 
{
#if (RISCV)
  return (double *)malloc(sizeof(double)*height*width);
#elif defined(__GNUC__) || defined(__GNUG__)
  return (double *)aligned_alloc(ALIGNMENT, sizeof(double)*height*width);
#elif defined(_MSC_VER)
  return (double *)_aligned_malloc(sizeof(double)*height*width, ALIGNMENT);
#else
#error UNKNOWN COMPILER
#endif
}

float * malloc_matrix_s(IDX height, IDX width) 
{
#if (RISCV)
  return (float*)malloc(sizeof(float)*height*width);
#elif defined(__GNUC__) || defined(__GNUG__)
  return (float *)aligned_alloc(ALIGNMENT, sizeof(float)*height*width);
#elif defined(_MSC_VER)
  return (float *)_aligned_malloc(sizeof(float)*height*width, ALIGNMENT);
#else
#error UNKNOWN COMPILER
#endif
}

void free_matrix_d(double *m)
{
#if defined(RISCV) || defined(__GNUC__) || defined(__GNUG__)
  free(m);
#elif defined(_MSC_VER)
  _aligned_free(m);
#else
#error UNKNOWN COMPILER
#endif
}

void free_matrix_s(float *m)
{
#if defined(RISCV) || defined(__GNUC__) || defined(__GNUG__)
  free(m);
#elif defined(_MSC_VER)
  _aligned_free(m);
#else
#error UNKNOWN COMPILER
#endif
}

//==========================================================================
// dump_matrix (double-precision)
//==========================================================================

void dump_matrix_dc(const char *name, double *A, IDX h, IDX w)
{
  IDX i, j;
  printf("%s (col-major)\n", name);
  printf("[\n");
  for (i = 0; i < h; ++i) {
    printf("  ");
    for (j = 0; j < w; ++j) {
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
  IDX i, j;
  printf("%s (row-major):\n", name);
  printf("[\n");
  for (i = 0; i < h; ++i) {
    printf("  ");
    for (j = 0; j < w; ++j) {
      if (j != 0) {
        printf(", ");
      }
      printf("%10.6f", IDXR(A, i, j, h, w));
    }
    printf("\n");
  }
  printf("]\n");
}

void dump_matrix_sc(const char *name, float *A, IDX h, IDX w)
{
  IDX i, j;
  printf("%s (col-major)\n", name);
  printf("[\n");
  for (i = 0; i < h; ++i) {
    printf("  ");
    for (j = 0; j < w; ++j) {
      if (j != 0) {
        printf(", ");
      }
      printf("%10.6f", IDXC(A, i, j, h, w));
    }
    printf("\n");
  }
  printf("]\n");
}

void dump_matrix_sr(const char *name, float *A, IDX h, IDX w)
{
  IDX i, j;
  printf("%s (row-major):\n", name);
  printf("[\n");
  for (i = 0; i < h; ++i) {
    printf("  ");
    for (j = 0; j < w; ++j) {
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
  IDX i, j;
  double *A = malloc_matrix_d(h, w);
  if (fill) {
    for (j = 0; j<w; ++j) {
      for (i = 0; i<h; ++i) {
        IDXC(A, i, j, h, w) = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return A;
}

//row major order
double * create_matrix_dr(IDX h, IDX w, bool fill)
{
  IDX i, j;
  double *A = malloc_matrix_d(h, w);
  if (fill) {
    for (i = 0; i<h; ++i) {
      for (j = 0; j < w; ++j) {
        IDXR(A, i, j, h, w) = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return A;
}

//col major order
float * create_matrix_sc(IDX h, IDX w, bool fill)
{
  IDX i, j;
  float *A = malloc_matrix_s(h, w);
  if (fill) {
    for (j = 0; j < w; ++j) {
      for (i = 0; i<h; ++i) {
        IDXC(A, i, j, h, w) = ((float)rand() / (float)RAND_MAX);
      }
    }
  }
  return A;
}

//row major order
float * create_matrix_sr(IDX h, IDX w, bool fill)
{
  IDX i, j;
  float *A = malloc_matrix_s(h, w);
  if (fill) {
    for (i = 0; i<h; ++i) {
      for (j = 0; j < w; ++j) {
        IDXR(A, i, j, h, w) = ((float)rand() / (float)RAND_MAX);
      }
    }
  }
  return A;
}

//==========================================================================
// create_spv_matrix (double-precision)
//==========================================================================

//col major order
//sparsity ranges from 0.0-1.0, where 0.0 is all zeros, and 1.0 is no zeros
//row_ptr: holds row offset of corresponding element in data_ptr array
//col_ptr: n'th index holds number of non-zeros up THROUGH the n'th column
void create_matrix_spv_dc(IDX h, IDX w, double sparsity, 
  double **data_ptr, uint32_t **row_ptr, uint32_t **col_ptr)
{
  IDX i, j;
  IDX total_non_zeros = (IDX)(h*w*sparsity);
  IDX current_non_zeros = 0;

  *data_ptr = (double *)malloc(total_non_zeros*sizeof(double));
  *row_ptr  = (uint32_t *)malloc(total_non_zeros*sizeof(uint32_t));
  *col_ptr  = (uint32_t *)malloc(w*sizeof(uint32_t));

  for (j = 0; j<w; ++j) {
    for (i = 0; i<h; ++i) {
      bool should_add = (((double)rand() / (double)RAND_MAX)) <= sparsity;
      if(should_add) {
        (*data_ptr)[current_non_zeros] = ((double)rand() / (double)RAND_MAX);
        (*row_ptr)[current_non_zeros]  = i;
        ++current_non_zeros;
        if(current_non_zeros == total_non_zeros) {
          break;
        }
      }
    }
    (*col_ptr)[j] = current_non_zeros;
    if(current_non_zeros == total_non_zeros) {
      break;
    }
  }
}

//row major order
//sparsity ranges from 0.0-1.0, where 0.0 is all zeros, and 1.0 is no zeros
//col_ptr: holds col offset of corresponding element in data_ptr array
//row_ptr: n'th index holds number of non-zeros up THROUGH the n'th row
void create_matrix_spv_dr(IDX h, IDX w, double sparsity, 
  double **data_ptr, uint32_t **col_ptr, uint32_t **row_ptr)
{
  IDX i, j, ii;
  IDX total_non_zeros = (IDX)(((double)(h*w))*sparsity);
  IDX current_non_zeros = 0;

  *data_ptr = (double *)malloc(total_non_zeros*sizeof(double));
  *col_ptr  = (uint32_t *)malloc(total_non_zeros*sizeof(uint32_t));
  *row_ptr  = (uint32_t *)malloc(w*sizeof(uint32_t));

  for (i = 0; i<h; ++i) {
    for (j = 0; j<w; ++j) {
      bool should_add = (((double)rand() / (double)RAND_MAX)) < sparsity;
      if(should_add) {
        (*data_ptr)[current_non_zeros] = ((double)rand() / (double)RAND_MAX);
        (*col_ptr)[current_non_zeros]  = j;
        ++current_non_zeros;
        if(current_non_zeros == total_non_zeros) {
          break;
        }
      }
    }
    (*row_ptr)[i] = current_non_zeros;
    if(current_non_zeros == total_non_zeros) {
      for(ii=i+1; ii<h; ++ii){
        (*row_ptr)[ii] = current_non_zeros;
      }
      break;
    }
  }
}

//==========================================================================
// dump_spv_matrix (double-precision)
//==========================================================================

void dump_matrix_spv_dc(const char *name, IDX h, IDX w,
  double *data_ptr, uint32_t *row_ptr, uint32_t *col_ptr)
{
  IDX i, j, offset_i;
  printf("%s (spv, col-major)\n", name);
  printf("[\n");
  for (j = 0; j < w; ++j) {
    printf("  ");
    IDX col_items = (j == 0 ? col_ptr[j] : col_ptr[j]-col_ptr[j-1]);
    IDX col_offset = (j == 0 ? 0 : col_ptr[j-1]);
    for (i = 0; i < h; ++i) {
      bool found = false;
      for (offset_i = col_offset; offset_i<(col_offset+col_items); ++offset_i) {
        if(row_ptr[offset_i] == i) {
          printf("%10.6lf,", data_ptr[offset_i]);
          found = true;
          break;
        }
      }
      if(!found) {
        printf("%10.6f,", 0.0);
      }
    }
    printf("\n");
  }
  printf("]\n");
}

void dump_matrix_spv_dr(const char *name, IDX h, IDX w,
  double *data_ptr, uint32_t *col_ptr, uint32_t *row_ptr)
{
  IDX i, j, offset_j;
  printf("%s (spv, row-major)\n", name);
  printf("data_ptr={");
  for(i=0; i<row_ptr[h-1]; ++i){
    printf("%6.3lf,", data_ptr[i]);
  }
  printf("}\n");
  printf("col_ptr= {");
  for(i=0; i<row_ptr[h-1]; ++i){
    printf("%6u,", col_ptr[i]);
  }
  printf("}\n");
  printf("row_ptr= {");
  for(i=0; i<h; ++i){
    printf("%6u,", row_ptr[i]);
  }
  printf("}\n");

  printf("expanded=[\n");
  for (i = 0; i < h; ++i) {
    printf("  ");
    IDX row_items = (i == 0 ? row_ptr[i] : row_ptr[i]-row_ptr[i-1]);
    IDX row_offset = (i == 0 ? 0 : row_ptr[i-1]);
    for (j = 0; j < w; ++j) {
      bool found = false;
      for (offset_j = row_offset; offset_j<(row_offset+row_items); ++offset_j) {
        if(col_ptr[offset_j] == j) {
          printf("%8.3lf,", data_ptr[offset_j]);
          found = true;
          break;
        }
      }
      if(!found) {
        printf("%8.3lf,", 0.0);
      }
    }
    printf("\n");
  }
  printf("]\n");
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
  IDX i, j;
  double *A = malloc_matrix_d(h, w);
  for (j = 0; j < w; ++j) {
    for (i = 0; i < h; ++i) {
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
  IDX i, j;
  double *A = malloc_matrix_d(h, w);
  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
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
  IDX i, j;
  double *B = malloc_matrix_d(h, w);
  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
      IDXC(B, i, j, h, w) = IDXC(A, i, j, h, w);
    }
  }
  return B;
}


// row major order
double * copy_matrix_dr(double *A, IDX h, IDX w)
{
  IDX i, j;
  double *B = malloc_matrix_d(h, w);
  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
      IDXR(B, i, j, h, w) = IDXR(A, i, j, h, w);
    }
  }
  return B;
}

// col major order
float * copy_matrix_sc(float *A, IDX h, IDX w)
{
  IDX i, j;
  float *B = malloc_matrix_s(h, w);
  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
      IDXC(B, i, j, h, w) = IDXC(A, i, j, h, w);
    }
  }
  return B;
}


// row major order
float * copy_matrix_sr(float *A, IDX h, IDX w)
{
  IDX i, j;
  float *B = malloc_matrix_s(h, w);
  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
      IDXR(B, i, j, h, w) = IDXR(A, i, j, h, w);
    }
  }
  return B;
}

//==========================================================================
// transpose_matrix (double-precision)
//==========================================================================

double *transpose_matrix_drc(double *A, IDX h, IDX w)
{
  IDX i, j;
  double *B = malloc_matrix_d(h, w);
  for (j = 0; j < w; ++j) {
    for (i = 0; i < h; ++i) {
      IDXC(B, i, j, h, w) = IDXR(A, i, j, h, w);
    }
  }
  return B;
}

double *transpose_matrix_dcr(double *A, IDX h, IDX w)
{
  IDX i, j;
  double *B = malloc_matrix_d(h, w);
  for (j = 0; j < w; ++j) {
    for (i = 0; i < h; ++i) {
      IDXR(B, i, j, h, w) = IDXC(A, i, j, h, w);
    }
  }
  return B;
}

float *transpose_matrix_src(float *A, IDX h, IDX w)
{
  IDX i, j;
  float *B = malloc_matrix_s(h, w);
  for (j = 0; j < w; ++j) {
    for (i = 0; i < h; ++i) {
      IDXC(B, i, j, h, w) = IDXR(A, i, j, h, w);
    }
  }
  return B;
}

float *transpose_matrix_scr(float *A, IDX h, IDX w)
{
  IDX i, j;
  float *B = malloc_matrix_s(h, w);
  for (j = 0; j < w; ++j) {
    for (i = 0; i < h; ++i) {
      IDXR(B, i, j, h, w) = IDXC(A, i, j, h, w);
    }
  }
  return B;
}


//col->row major order in place
void transpose_matrix_in_place_drc(double *A, IDX h, IDX w)
{
  double *copy = transpose_matrix_drc(A, h, w);
  memcpy((char *)A, (char *)copy, sizeof(double)*h*w);
  free_matrix_d(copy);
}

//col->row major order in place
void transpose_matrix_in_place_dcr(double *A, IDX h, IDX w)
{
  double *copy = transpose_matrix_dcr(A, h, w);
  memcpy((char *)A, (char *)copy, sizeof(double)*h*w);
  free_matrix_d(copy);
}

//col->row major order in place
void transpose_matrix_in_place_src(float *A, IDX h, IDX w)
{
  float *copy = transpose_matrix_src(A, h, w);
  memcpy((char *)A, (char *)copy, sizeof(float)*h*w);
  free_matrix_s(copy);
}

//col->row major order in place
void transpose_matrix_in_place_scr(float *A, IDX h, IDX w)
{
  float *copy = transpose_matrix_scr(A, h, w);
  memcpy((char *)A, (char *)copy, sizeof(float)*h*w);
  free_matrix_s(copy);
}

//==========================================================================
// converting single<-->double
//==========================================================================

double * matrix_s_to_d(float *A, IDX h, IDX w)
{
  IDX i;
  double *copy = malloc_matrix_d(h, w);
  for (i = 0; i < h*w; ++i) {
    copy[i] = (double) A[i];
  }
  return copy;
}

//==========================================================================
// miscellaneous calls
//==========================================================================

double get_scalar_double()
{
  return ((double)rand() / (double)RAND_MAX);
}
