//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Matrix Utility functions for creating/destroying and manipulating matrices
// in column-order or row-order in memory. Additionally, provides some other
// miscellaneous things such as FLOP equation macros, and matrix indexing
// macros
//==========================================================================

#ifndef __MATRIX_UTILITIES_H__
#define __MATRIX_UTILITIES_H__

#include <stdbool.h>
#include <stdint.h>

#include "matrix_types.h"

//==========================================================================
// MATRIX UTILITY FUNCTIONS
//
// if it ends in *_sX it means its single precision
// if it ends in *_dX it means its double precision
// if it ends in *_Xr it means it works with matrices in row-major order
// if it ends in *_Xc it means it works with matrices in col-major order
//==========================================================================

// Platform-independent aligned-malloc/free
double * malloc_matrix_d(IDX height, IDX width);
float * malloc_matrix_s(IDX height, IDX width);
//float * malloc_matrix_f(IDX height, IDX width) 
void free_matrix_d(double *m);
void free_matrix_s(float *m);
//void free_matrix_f(float *m)

//prints the matrix contents to the terminal
void dump_matrix_dc(const char *name, double *A, IDX h, IDX w);
void dump_matrix_dr(const char *name, double *A, IDX h, IDX w);
void dump_matrix_sc(const char *name, float *A, IDX h, IDX w);
void dump_matrix_sr(const char *name, float *A, IDX h, IDX w);

//create_matrix constanst
#define FILL 1
#define NO_FILL 0

//fill == true if you want the matrix filled with data. column/row ordering
//versions of function both exist. Always use this function when creating 
//matrices, since it will align them in memory nicely
//float * create_matrix_sc(IDX h, IDX w, bool fill);
//float * create_matrix_sr(IDX h, IDX w, bool fill);
double * create_matrix_dc(IDX h, IDX w, bool fill);
double * create_matrix_dr(IDX h, IDX w, bool fill);
float * create_matrix_sc(IDX h, IDX w, bool fill);
float * create_matrix_sr(IDX h, IDX w, bool fill);


//==============================================================================
// Sparse Stuff
//==============================================================================

void create_matrix_spv_dc(IDX h, IDX w, double sparsity, 
  double **data_ptr, uint32_t **row_ptr, uint32_t **col_ptr);
void create_matrix_spv_dr(IDX h, IDX w, double sparsity, 
  double **data_ptr, uint32_t **col_ptr, uint32_t **row_ptr);

void dump_matrix_spv_dc(const char *name, IDX h, IDX w,
  double *data_ptr, uint32_t *row_ptr, uint32_t *col_ptr);
void dump_matrix_spv_dr(const char *name, IDX h, IDX w,
  double *data_ptr, uint32_t *col_ptr, uint32_t *row_ptr);

//==============================================================================

//triangular matrix related constants
#define TRI_UPPER 1
#define TRI_LOWER 0
#define TRI_UNIT 1
#define TRI_NON_UNIT 0

// creates triangular matrix with row/column ordering.
//float * create_matrix_trianglular_sc(IDX h, IDX w, bool upper, bool unit);
//float * create_matrix_trianglular_sr(IDX h, IDX w, bool upper, bool unit);
double * create_matrix_trianglular_dc(IDX h, IDX w, bool upper, bool unit);
double * create_matrix_trianglular_dr(IDX h, IDX w, bool upper, bool unit);

// copy matrix into new memory location, preserving row/col ordering
//float * copy_matrix_sc(float *A, IDX h, IDX w);
//float * copy_matrix_sr(float *A, IDX h, IDX w);
double * copy_matrix_dc(double *A, IDX h, IDX w);
double * copy_matrix_dr(double *A, IDX h, IDX w);
float * copy_matrix_sc(float *A, IDX h, IDX w);
float * copy_matrix_sr(float *A, IDX h, IDX w);

// creates new matrix, transposed. if  row/col ordering remain the same,this
// performs the correct transpose
//float * transpose_matrix_s(double *A, IDX h, IDX w);
double * transpose_matrix_drc(double *A, IDX h, IDX w);
double * transpose_matrix_dcr(double *A, IDX h, IDX w);
float * transpose_matrix_src(float *A, IDX h, IDX w);
float * transpose_matrix_scr(float *A, IDX h, IDX w);
void transpose_matrix_in_place_drc(double *A, IDX h, IDX w);
void transpose_matrix_in_place_dcr(double *A, IDX h, IDX w);
void transpose_matrix_in_place_src(float *A, IDX h, IDX w);
void transpose_matrix_in_place_scr(float *A, IDX h, IDX w);

// just a wrapper around transpose matrix that makes the intention clearer
//float * swap_row_col_ordering_s(double *A, IDX h, IDX w);
//double * swap_row_col_ordering_d(double *A, IDX h, IDX w);

double * matrix_s_to_d(float *A, IDX h, IDX w);

//==============================================================================
// SCALAR FUNCTIONS
//==============================================================================

// generates a random scalar
float get_scalar_float();
double get_scalar_double();

#endif // __MATRIX_UTILITIES_H__
