

#include "matrix_utilities.h"

#define N_MIN 32
#define N_MAX 2048
#define N_SCALE 2

#define P_MIN 1
#define P_MAX 1024
#define P_SCALE 4

//==========================================================================
// LU Factorization With separate A, L, U matrices with 
// col_major/row_major/col_major ordering 
//==========================================================================

//A=col major, L=row major, U=col major
void l_iteration(double *A, double *L, double *U, IDX N, IDX i)
{
  for(IDX j=(i+1); j<N; ++j){
    double *Lji = &(IDXR(L, j, i, N, N));
    double Aji = IDXC(A, j, i, N, N);
    double Uii = IDXC(U, i, i, N, N);

    IDX k=0;
    IDX k_left = i;
    double *Ljk = &(IDXR(L, j, k, N, N));
    double *Uki = &(IDXC(U, k, i, N, N));

    *Lji = Aji;
    for(k; k<k_left; ++k){
      *Lji -= Ljk[k] * Uki[k];
    }
    *Lji /= Uii;
  }
}

//A=col major, L=row major, U=col major
void u_iteration(double *A, double *L, double *U, IDX N, IDX i)
{
  for (IDX j = (i + 1); j<N; ++j) {
    double *Uip1j = &(IDXC(U, i+1, j, N, N));
    double Aip1j = IDXC(A, i+1, j, N, N);

    IDX k = 0;
    IDX k_left = i;
    double *Lip1k = &(IDXR(L, i+1, k, N, N));
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
  for(IDX i=0; i<N; ++i){
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
      double Lip1k =IDXC(A, i + 1, k, N, N);
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
  for(IDX col=0; col<N; ++col){
    Pvec[col] = col;
  }
  //perform permutations and record swaps
  for(IDX col=0; col<N; ++col){
    double max_val = IDXC(A, col, col, N, N);
    IDX max_val_row = col;
    for(IDX row=col+1; row<N; ++row){
      double new_val = IDXC(A, row, col, N, N);
      if(new_val > max_val){
        max_val = new_val;
        max_val_row = col;
      }
    }
    if(max_val_row != col){
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


void lu_solve_d(double *A, double *B, IDX N, IDX P)
{
  IDX *Pvec = lu_permute_in_place(A, N);
  IDX *Pvec_trans = lu_transpose_permute_vec(Pvec, N);
  apply_permute_vec(B, Pvec_trans, N, P);
  lu_decompose_in_place(A, N);
  do_trsm(A, B, N, N, P, LOWER, LEFT, UNIT, NO_TRANS);
  do_trsm(A, B, N, N, P, UPPER, RIGHT, NO_UNIT, NO_TRANS);
  do_trsm(A_actual, B, M, B_rows, B_cols, right, real_upper, unit, alpha);
}


//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//
//see trsm notes: 
//  for left-side A-matrix, B is col major and A is row major.
//  this actually plays nicely with the row swapping in A anyways
//Also, gsl uses column order for B anyways, since it only solves one column
//  at a time, so we can leave B as a col-major for verify_lu_solve_d
//==========================================================================
void do_test(IDX N, IDX P)
{
  double *A = create_matrix_dr(N, N, FILL);
  double *B = create_matrix_dc(N, P, FILL);

  double *A_orig = copy_matrix_dr(A, N, N);
  double *B_orig = copy_matrix_dc(B, N, P);

  double *gsl_A = copy_matrix_dr(A_orig, N, N);
  double *gsl_B = copy_matrix_dc(B_orig, N, P);

  double start = getRealTime();
  lu_solve_d(A, B, N, P);
  double duration = getRealTime() - start;

  double flop = TRSM_FLOP_UNIT(N, P) + TRSM_FLOP_DIAG(N, P) + LU_FLOP(N);
  printf("N=%4u P=%4u | took % 10.6f secs | ", N, P, duration);
  printf("% 10.3f MFLOP | ", flop / 1e6);
  printf("% 10.3f MFLOP/s || ", flop / (duration*1e6));

  verify_gsl_lu_solve_d(gsl_A, gsl_B, A_orig, B_orig, A, B, N, P);

  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(A_orig);
  destroy_matrix_d(B_orig);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
}


//==========================================================================
// TEST ENTRYPOINT -- TEST SWEEPS MULTIPLE PARAMETERS (all 16 versions)
//==========================================================================

int main(int argc, char **argv)
{
  //set breakpoint here, then make command window huge
  srand((unsigned)time(NULL));

  for (IDX N = N_MIN; N <= N_MAX; N *= N_SCALE) {
    for (IDX P = P_MIN; P <= P_MAX; P *= P_SCALE) {
      do_test(N, P);
    }
  }
  printf("\n");

  //set breakpoint here, then analyze results
  return 0;
}