#ifndef __LU_SOLVE_H__
#define __LU_SOLVE_H__

#include <intrin.h>

#include "matrix_utilities.h"

//block sizes are just roughly picked. memory systems change from machine to
//machine too much to make a difference
#define BLOCK_D 64
#define BLOCK_S 128

//==========================================================================
// LEFT LOWER KERNELS (traverse top-left to bottom right)
//==========================================================================

//==========================================================================
// LU Factorization With separate A, L, U matrices with 
// col_major/row_major/col_major ordering 
//==========================================================================

//A=col major, L=row major, U=col major
void l_iteration(double *A, double *L, double *U, IDX N, IDX i)
{
  for (IDX j = (i + 1); j<N; ++j) {
    double *Lji = &(IDXR(L, j, i, N, N));
    double Aji = IDXC(A, j, i, N, N);
    double Uii = IDXC(U, i, i, N, N);

    IDX k = 0;
    IDX k_left = i;
    double *Ljk = &(IDXR(L, j, k, N, N));
    double *Uki = &(IDXC(U, k, i, N, N));

    *Lji = Aji;
    for (k; k<k_left; ++k) {
      *Lji -= Ljk[k] * Uki[k];
    }
    *Lji /= Uii;
  }
}

//A=col major, L=row major, U=col major
void u_iteration(double *A, double *L, double *U, IDX N, IDX i)
{
  for (IDX j = (i + 1); j<N; ++j) {
    double *Uip1j = &(IDXC(U, i + 1, j, N, N));
    double Aip1j = IDXC(A, i + 1, j, N, N);

    IDX k = 0;
    IDX k_left = i;
    double *Lip1k = &(IDXR(L, i + 1, k, N, N));
    double *Ukj = &(IDXC(U, k, j, N, N));

    *Uip1j = Aip1j;
    for (k; k<k_left; ++k) {
      *Uip1j -= Lip1k[k] * Ukj[k];
    }
  }
}

void lu_decompose(double *A, double *L, double *U, IDX N)
{
  //initialize matrices
  for (IDX i = 0; i<N; ++i) {
    IDXR(L, i, i, N, N) = 1;
    IDXC(U, 0, i, N, N) = IDXC(A, 0, i, N, N);
  }

  for (IDX i = 0; i < N - 1; ++i) {
    l_iteration(A, L, U, N, i);
    u_iteration(A, L, U, N, i);
  }
}


//==========================================================================
// In place LU Factorization
//==========================================================================

//A=col major order
void l_iteration_in_place(double *A, IDX N, IDX i)
{
  for (IDX j = (i + 1); j<N; ++j) {
    double *Lji = &(IDXC(A, j, i, N, N));
    double Uii = IDXC(A, i, i, N, N);

    for (IDX k; k<i; ++k) {
      double Ljk = IDXC(A, j, k, N, N);
      double Uki = IDXC(A, k, i, N, N);
      *Lji -= Ljk * Uki;
    }
    *Lji /= Uii;
  }
}

//A=col major, L=row major, U=col major
void u_iteration_in_place(double *A, double *U, IDX N, IDX i)
{
  for (IDX j = (i + 1); j<N; ++j) {
    double *Uip1j = &(IDXC(U, i + 1, j, N, N));

    for (IDX k; k<i; ++k) {
      double Lip1k = IDXC(A, i + 1, k, N, N);
      double Ukj = IDXC(A, k, j, N, N);
      *Uip1j -= Lip1k * Ukj;
    }
  }
}

void lu_decompose_in_place(double *A, IDX N)
{
  //initialize matrices
  for (IDX i = 0; i < N - 1; ++i) {
    l_iteration_in_place(A, N, i);
    u_iteration_in_place(A, N, i);
  }
}

//==========================================================================
// In place LU Factorization
//==========================================================================

//row major order
void swap_rows_dr(double *A, IDX h, IDX w, IDX r1, IDX r2)
{
  double *tmp = (double *)_aligned_malloc(sizeof(double)*w, 64);
  double *r1_start = &(IDXR(A, r1, 0, h, w));
  double *r2_start = &(IDXR(A, r2, 0, h, w));
  memcpy(tmp, r1_start, sizeof(double)*w);
  memcpy(r1_start, r2_start, sizeof(double)*w);
  memcpy(r2_start, tmp, sizeof(double)*w);
}

void swap_rows_dc(double *A, IDX h, IDX w, IDX r1, IDX r2)
{
  for (IDX i = 0; i < w; i++) {
    double tmp = IDXC(A, r1, i, h, w);
    IDXC(A, r1, i, h, w) = IDXC(A, r2, i, h, w);
    IDXC(A, r2, i, h, w) = tmp;
  }
}

//A is column major order
IDX* lu_permute_in_place(double *A, IDX N)
{
  IDX *Pvec = (IDX *)_aligned_malloc(sizeof(IDX)*N, 64);

  //initialize permutation vector
  for (IDX col = 0; col<N; ++col) {
    Pvec[col] = col;
  }
  //perform permutations and record swaps
  for (IDX col = 0; col<N; ++col) {
    double max_val = IDXC(A, col, col, N, N);
    IDX max_val_row = col;
    for (IDX row = col + 1; row<N; ++row) {
      double new_val = IDXC(A, row, col, N, N);
      if (new_val > max_val) {
        max_val = new_val;
        max_val_row = col;
      }
    }
    if (max_val_row != col) {
      IDX tmp = Pvec[col];
      Pvec[col] = Pvec[max_val_row];
      Pvec[max_val_row] = tmp;
      swap_rows_dr(A, N, N, col, max_val_row);
    }
  }
  return Pvec;
}

IDX * lu_transpose_permute_vec(IDX *Pvec, IDX N)
{
  IDX *Pvec_trans = (IDX *)_aligned_malloc(sizeof(IDX)*N, 64);
  for (IDX i = 0; i < N; ++i) {
    Pvec_trans[Pvec[i]] = i;
  }
  return Pvec_trans;
}

void apply_permute_vec(double *A, IDX *Pvec, IDX rows, IDX cols)
{
  for (IDX i = 0; i < rows; ++i) {
    if (Pvec[i] != i) {
      swap_rows(A, rows, cols, i, Pvec[i]);
    }
  }
}

#endif //__LU_SOLVE_H__