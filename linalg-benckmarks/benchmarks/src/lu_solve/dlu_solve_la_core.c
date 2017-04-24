//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of Complex double-precision FFT of
// arbitrary-length vectors
//==========================================================================

#include "lu_solve/dlu_solve_la_core.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "trsm/dtrsm_la_core.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

extern bool DEBUG;
extern uint64_t SCRATCH_SIZE;

static const IDX DPSIZE = sizeof(double);

//==========================================================================
// PIVOTING AND SWAP ROWS
//==========================================================================

//row major order
static void la_swap_rows_dr(double *A, IDX h, IDX w, IDX r1, IDX r2)
{
  const IDX BLOCK = SCRATCH_SIZE / sizeof(double);

  const LaRegIdx tmp_sch_REG = 0;
  const LaRegIdx r1_mem_REG  = 1;
  const LaRegIdx r2_mem_REG  = 2;

  la_set_vec_adr_dp_sch(tmp_sch_REG, 0);
  for(IDX i=0; i<w; i+=BLOCK) {
    IDX icount = MIN(BLOCK, (w-i));
    LaAddr r1_istart = (LaAddr) &(IDXR(A, r1, i, h, w));
    LaAddr r2_istart = (LaAddr) &(IDXR(A, r2, i, h, w));

    la_set_vec_adr_dp_mem(r1_mem_REG, r1_istart);
    la_set_vec_adr_dp_mem(r2_mem_REG, r2_istart);

    la_copy(tmp_sch_REG, r1_mem_REG, icount);
    la_copy(r1_mem_REG, r2_mem_REG, icount);
    la_copy(r2_mem_REG, tmp_sch_REG, icount);
  }
}

//col major order
static void la_swap_rows_dc(double *A, IDX h, IDX w, IDX r1, IDX r2)
{
  double tmp = IDXC(A, r1, 0, h, w);
  IDXC(A, r1, 0, h, w) = IDXC(A, r2, 0, h, w);
  IDXC(A, r2, 0, h, w) = tmp;

  //const IDX BLOCK = SCRATCH_SIZE / sizeof(double);
  /*
  IDX BLOCK = 8;

  const LaRegIdx tmp_sch_REG = 0;
  const LaRegIdx r1_mem_REG  = 1;
  const LaRegIdx r2_mem_REG  = 2;

  la_set_vec_adr_dp_sch(tmp_sch_REG, 0);

  for(IDX i=0; i<w; i+=BLOCK) {
    IDX icount = MIN(BLOCK, (w-i));
    LaAddr r1_istart = (LaAddr) &(IDXC(A, r1, i, h, w));
    LaAddr r2_istart = (LaAddr) &(IDXC(A, r2, i, h, w));

    la_set_vec_dp_mem(r1_mem_REG, r1_istart, h, icount, 0);
    la_set_vec_dp_mem(r2_mem_REG, r2_istart, h, icount, 0);

    la_copy(tmp_sch_REG, r1_mem_REG, icount);
    la_copy(r1_mem_REG, r2_mem_REG, icount);
    la_copy(r2_mem_REG, tmp_sch_REG, icount);
  }
  */
}

//A in row-major, B in col-major order
static void lu_permute_in_place(double *A, double *B, IDX iter, IDX N, IDX P)
{
  IDX col = iter;
  double max_val = IDXR(A, col, col, N, N);
  IDX max_val_row = col;
  for(IDX row=col+1; row<N; ++row){
    double new_val = IDXR(A, row, col, N, N);
    if(new_val > max_val){
      max_val = new_val;
      max_val_row = row;
    }
  }
  if(max_val_row != col){
    la_swap_rows_dr(A, N, N, col, max_val_row);
    la_swap_rows_dc(B, N, P, col, max_val_row);
  }
}


//==========================================================================
// In place LU Factorization
//==========================================================================

//A in row-major order
void l_iteration_in_place(double *A, IDX N, IDX iter)
{
  IDX i       = iter+1;
  IDX j       = iter;
  IDX icount  = N-i;
  IDX kcount  = j;
  IDX ikcount = icount*kcount;

  const LaAddr U0J_SCH_ADR        = 0;
  const LaAddr LI0_U0J_SCH_ADR    = kcount*sizeof(double);

  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx Ujj_REG          = 1;
  const LaRegIdx U0j_mem_REG      = 2;
  const LaRegIdx U0j_sch_REG      = 3;
  const LaRegIdx Li0_mem_REG      = 4; 
  const LaRegIdx Lij_mem_REG      = 5;
  const LaRegIdx Li0_U0j_sch_REG  = 6;

  LaAddr Lij_start  = (LaAddr) &(IDXR(A, i, j, N, N));
  LaAddr Li0_start  = (LaAddr) &(IDXR(A, i, 0, N, N));
  LaAddr U0j_start  = (LaAddr) &(IDXR(A, 0, j, N, N));
  double Ujj        =            IDXR(A, j, j, N, N);

  if(iter > 0) {
    //copy U0i-Uji to scratchpad
    la_set_vec_dp_mem(U0j_mem_REG, U0j_start, N, kcount, 0);
    la_set_vec_dp_sch(U0j_sch_REG, U0J_SCH_ADR, 1, kcount, -kcount);
    la_copy(U0j_sch_REG, U0j_mem_REG, kcount);

    //multi-reduce Lj0 with U0i-repeated and put in scratchpad
    la_set_scalar_dp_reg(Zero_REG, 0.0);
    la_set_vec_dp_mem(Li0_mem_REG, Li0_start, 1, kcount, (N-kcount));
    la_set_vec_dp_sch(Li0_U0j_sch_REG, LI0_U0J_SCH_ADR, 1, icount, 0);
    la_AaddBmulC_sum_multi(Li0_U0j_sch_REG, 
                           Li0_mem_REG, Zero_REG, U0j_sch_REG, ikcount);

    //subtract results from Lji vector and divide by Uii
    la_set_scalar_dp_reg(Ujj_REG, Ujj);
    la_set_vec_dp_mem(Lij_mem_REG, Lij_start, N, icount, 0);
    la_AsubBdivC(Lij_mem_REG, Lij_mem_REG, Li0_U0j_sch_REG, Ujj_REG, icount);
  } else {
    la_set_scalar_dp_reg(Zero_REG, 0.0);
    la_set_scalar_dp_reg(Ujj_REG, Ujj);
    la_set_vec_dp_mem(Lij_mem_REG, Lij_start, N, icount, 0);
    la_AaddBdivC(Lij_mem_REG, Lij_mem_REG, Zero_REG, Ujj_REG, icount);
  }
}

//A in row-major order
void u_iteration_in_place(double *A, IDX N, IDX iter)
{
  if(iter == 0) {
    return;
  }

  IDX i       = iter;
  IDX j       = iter;
  IDX jcount  = N-i;
  IDX kcount  = j;
  IDX jkcount = jcount*kcount;

  const LaAddr LI0_SCH_ADR        = 0;
  const LaAddr LI0_U0J_SCH_ADR    = kcount*sizeof(double);

  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx One_REG          = 1;
  const LaRegIdx Li0_mem_REG      = 2;
  const LaRegIdx Li0_sch_REG      = 3;
  const LaRegIdx U0j_mem_REG      = 4; 
  const LaRegIdx Uij_mem_REG      = 5;
  const LaRegIdx Li0_U0j_sch_REG  = 6;

  LaAddr Li0_start  = (LaAddr) &(IDXR(A, i, 0, N, N));
  LaAddr U0j_start  = (LaAddr) &(IDXR(A, 0, j, N, N));
  LaAddr Uij_start  = (LaAddr) &(IDXR(A, i, j, N, N));

  //copy Lii-LiN to scratchpad
  la_set_vec_dp_mem(Li0_mem_REG, Li0_start, 1, kcount, 0);
  la_set_vec_dp_sch(Li0_sch_REG, LI0_SCH_ADR, 1, kcount, -kcount);
  la_copy(Li0_sch_REG, Li0_mem_REG, kcount);

  //multi-reduce Li0 with U0j-repeated and put in scratchpad
  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_vec_dp_mem(U0j_mem_REG, U0j_start, N, kcount, (1-N*kcount));
  la_set_vec_dp_sch(Li0_U0j_sch_REG, LI0_U0J_SCH_ADR, 1, jcount, 0);
  la_AaddBmulC_sum_multi(Li0_U0j_sch_REG, 
                         Li0_sch_REG, Zero_REG, U0j_mem_REG, jkcount);

  //subtract results from Uji vector and mul by 1.0
  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_vec_dp_mem(Uij_mem_REG, Uij_start, 1, jcount, 0);
  la_AsubBmulC(Uij_mem_REG, Uij_mem_REG, Li0_U0j_sch_REG, One_REG, jcount);
}

//A in row-major
void lu_decompose_in_place(double *A, double *B, IDX N, IDX P)
{
  for (IDX iter = 0; iter < N; ++iter) {
    lu_permute_in_place(A, B, iter, N, P);
    u_iteration_in_place(A, N, iter);
    l_iteration_in_place(A, N, iter);
  }
  if(DEBUG) {
    printf("L_solved=[\n");
    for(IDX i=0; i<N; ++i) {
      printf(" ");
      for(IDX j=0; j<N; ++j) {
        if(j>i)       printf("%11.6f,", 0.0);
        else if(j==i) printf("%11.6f,", 1.0);
        else          printf("%11.6f,", IDXR(A, i, j, N, N));
      }
      printf("\n");
    }
    printf("]\n");

    printf("U_solved=[\n");
    for(IDX i=0; i<N; ++i) {
      printf(" ");
      for(IDX j=0; j<N; ++j) {
        if(j<i)       printf("%11.6f,", 0.0);
        else          printf("%11.6f,", IDXR(A, i, j, N, N));
      }
      printf("\n");
    }
    printf("]\n");

    dump_matrix_dc("B_permuted", B, N, P);
  }
}
//==========================================================================
// LU SOLVE MAIN
//==========================================================================

static const bool RIGHT     = true;
static const bool LEFT      = false;
static const bool UPPER     = true;
static const bool LOWER     = false;
static const bool UNIT      = true;
static const bool NO_UNIT   = false;

//A row-major and B col-major
void dlu_solve_la_core(double *A, double *B, IDX N, IDX P)
{
  if(DEBUG) {
    dump_matrix_dr("A_orig", A, N, N);
    dump_matrix_dc("B_orig", B, N, P);
  }
  lu_decompose_in_place(A, B, N, P);
  dtrsm_la_core(A, B, N, P, LEFT, LOWER, UNIT, 1.0);
  dtrsm_la_core(A, B, N, P, LEFT, UPPER, NO_UNIT, 1.0);
}












//==========================================================================
// In place LU Factorization
//==========================================================================

//4096 should be enough...
double U_vec[4096];

const LaAddr U_VEC_SCH_ADR   = 0;
const LaRegIdx U_vec_mem_REG = 6;
const LaRegIdx U_vec_sch_REG = 7;

//A in row-major order
void l_iteration_in_place_fast(double *A, IDX N, IDX *pvec, IDX iter)
{
  IDX i       = iter+1;
  IDX j       = iter;
  IDX icount  = N-i;
  IDX kcount  = j;

  const LaRegIdx Zero_REG     = 0;
  const LaRegIdx Ujj_REG      = 1;
  const LaRegIdx U0j_mem_REG  = 2;
  const LaRegIdx U0j_sch_REG  = 3;
  const LaRegIdx Li0_mem_REG  = 4; 
  const LaRegIdx Lij_mem_REG  = 5;

  double Ujj = IDXR(A, pvec[j], j, N, N);

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  //la_set_scalar_dp_reg(Ujj_REG, Ujj);

  if(iter > 0) {
    for(IDX ii=i; ii<N; ++ii){
      LaAddr Li0 = (LaAddr) &(IDXR(A, pvec[ii], 0, N, N));
      LaAddr Lij = (LaAddr) &(IDXR(A, pvec[ii], j, N, N));

      la_set_vec_adr_dp_mem(Li0_mem_REG, Li0);
      la_set_scalar_dp_mem(Lij_mem_REG, Lij);
      la_AaddBmulC_sum(Lij_mem_REG, U_vec_sch_REG, Zero_REG, Li0_mem_REG, kcount+1);
    }
  } 
  //else if(pvec[0] == 0) {
    //divide whole first column (except for first row) by U00
    //LaAddr Lij = (LaAddr) &(IDXR(A, 1, j, N, N));
    //la_set_vec_dp_mem(Lij_mem_REG, (LaAddr)Lij, N, 1, 0);
    //la_AaddBdivC(Lij_mem_REG, Lij_mem_REG, Zero_REG, Ujj_REG, N-1);
  else {
    for(IDX i=1; i<N; ++i){
      IDXR(A, pvec[i], j, N, N) /= Ujj;
    }
  }
  /*} 
  else {
    //first row was swapped, need to perform 2 divides by U00 now
    IDX count1 = pvec[0];
    IDX count2 = N-1-count1;

    LaAddr Lij = (LaAddr) &(IDXR(A, 0, j, N, N));
    la_set_vec_dp_mem(Lij_mem_REG, (LaAddr)Lij, N, 1, 0);
    la_AaddBdivC(Lij_mem_REG, Lij_mem_REG, Zero_REG, Ujj_REG, count1);

    Lij = (LaAddr) &(IDXR(A, pvec[0]+1, j, N, N));
    la_set_vec_dp_mem(Lij_mem_REG, (LaAddr)Lij, N, 1, 0);
    la_AaddBdivC(Lij_mem_REG, Lij_mem_REG, Zero_REG, Ujj_REG, count2);
  }*/
}

//A in row-major order
void u_iteration_in_place_fast(double *A, IDX N, IDX *pvec, IDX iter)
{
  if(iter > 0) {
    IDX i       = iter;
    IDX j       = iter;
    IDX jcount  = N-j;
    IDX kcount  = j;

    const LaAddr ACCUM_SCH_ADR  = 0;

    const LaRegIdx Zero_REG     = 0;
    const LaRegIdx One_REG      = 1;
    const LaRegIdx Lik_REG      = 2;
    const LaRegIdx Ukj_mem_REG  = 3;
    const LaRegIdx Uij_mem_REG  = 4;
    const LaRegIdx Accum_REG    = 5;

    LaAddr Uij_start  = (LaAddr) &(IDXR(A, pvec[i], j, N, N));

    //zero scratchpad accumulators
    la_set_scalar_dp_reg(Zero_REG, 0.0);
    la_set_vec_adr_dp_sch(Accum_REG, ACCUM_SCH_ADR);
    la_copy(Accum_REG, Zero_REG, jcount);

    for(IDX k=0; k<kcount; ++k){
      LaAddr Ukj  = (LaAddr) &(IDXR(A, pvec[k], j, N, N));
      LaAddr Lik  = (LaAddr) &(IDXR(A, pvec[i], k, N, N));

      U_vec[k] = *(double *)Ukj; //set up for L iteration

      la_set_scalar_dp_mem(Lik_REG, Lik);
      la_set_vec_adr_dp_mem(Ukj_mem_REG, Ukj);
      la_AmulBaddC(Accum_REG, Ukj_mem_REG, Lik_REG, Accum_REG, jcount);
    }

    //subtract results from Uji vector and mul by 1.0
    la_set_scalar_dp_reg(One_REG, 1.0);
    la_set_vec_adr_dp_mem(Uij_mem_REG, Uij_start);
    la_AsubBmulC(Uij_mem_REG, Uij_mem_REG, Accum_REG, One_REG, jcount);

    //set up for L iteration
    U_vec[kcount] = -1.0;
    la_set_vec_adr_dp_mem(U_vec_mem_REG, (LaAddr)U_vec);
    la_set_vec_adr_dp_sch(U_vec_sch_REG, U_VEC_SCH_ADR);
    la_set_scalar_dp_mem(Uij_mem_REG, Uij_start);
    la_AsubBdivC(U_vec_sch_REG, Zero_REG, U_vec_mem_REG, Uij_mem_REG, kcount+1);
  }
}


//A in row-major, B in col-major order
static void lu_permute_in_place_fast(double *A, double *B, IDX *pvec,
  IDX iter, IDX N)
{
  IDX col = iter;
  double max_val = IDXR(A, pvec[col], col, N, N);
  IDX max_val_row = col;
  for(IDX row=col+1; row<N; ++row){
    double new_val = IDXR(A, pvec[row], col, N, N);
    if(new_val > max_val){
      max_val = new_val;
      max_val_row = row;
    }
  }
  if(max_val_row != col){
    IDX tmp = pvec[col];
    pvec[col] = pvec[max_val_row];
    pvec[max_val_row] = tmp;

    double tmp_B = B[col];
    B[col] = B[max_val_row];
    B[max_val_row] = tmp_B;
  }
}


//A in row-major
void lu_decompose_in_place_fast(double *A, double *B, IDX *pvec, IDX N)
{
  for (IDX iter = 0; iter < N; ++iter) {
    lu_permute_in_place_fast(A, B, pvec, iter, N);
    u_iteration_in_place_fast(A, N, pvec, iter);
    l_iteration_in_place_fast(A, N, pvec, iter);
  }
  if(DEBUG) {
    printf("L_solved=[\n");
    for(IDX i=0; i<N; ++i) {
      printf(" ");
      for(IDX j=0; j<N; ++j) {
        if(j>i)       printf("%11.6f,", 0.0);
        else if(j==i) printf("%11.6f,", 1.0);
        else          printf("%11.6f,", IDXR(A, pvec[i], j, N, N));
      }
      printf("\n");
    }
    printf("]\n");

    printf("U_solved=[\n");
    for(IDX i=0; i<N; ++i) {
      printf(" ");
      for(IDX j=0; j<N; ++j) {
        if(j<i)       printf("%11.6f,", 0.0);
        else          printf("%11.6f,", IDXR(A, pvec[i], j, N, N));
      }
      printf("\n");
    }
    printf("]\n");

    dump_matrix_dc("B_permuted", B, N, 1);
  }
}

//assumes P == 1, for HPL!
void left_lower_fast(double *A, double *B, IDX *pvec, IDX M, double alpha)
{
  const LaAddr B_SCH_ADR      = 0;
  const LaAddr X_SCH_ADR      = SCRATCH_SIZE/2;

  const LaRegIdx Zero_REG       = 0;
  const LaRegIdx One_REG        = 1;
  const LaRegIdx Alpha_REG      = 2;
  const LaRegIdx X_src_sch_REG  = 3;
  const LaRegIdx X_dst_sch_REG  = 4;
  const LaRegIdx B_mem_REG      = 5;
  const LaRegIdx B_sch_REG      = 6;
  const LaRegIdx Aj0_mem_REG    = 7;

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  //precompute alpha*Bjk
  la_set_vec_adr_dp_mem(B_mem_REG, (LaAddr)B);
  la_set_vec_adr_dp_sch(B_sch_REG, B_SCH_ADR);
  la_AmulBaddC(B_sch_REG, Alpha_REG, B_mem_REG, Zero_REG, M);

  //zero initial element
  la_set_vec_adr_dp_sch(X_src_sch_REG, X_SCH_ADR);
  la_copy(X_src_sch_REG, Zero_REG, 1);

  //solve each item in B column at a time
  LaAddr X_dst_adr = X_SCH_ADR+DPSIZE;
  for(IDX j=1; j<M; ++j) {
    LaAddr Aj0 = (LaAddr) &IDXR(A, pvec[j], 0, M, M);

    la_set_vec_adr_dp_mem(Aj0_mem_REG, Aj0);
    la_set_scalar_dp_sch(X_dst_sch_REG, X_dst_adr);
    la_AsubBmulC_sum(X_dst_sch_REG, B_sch_REG, X_src_sch_REG, Aj0_mem_REG, j);
    X_dst_adr += DPSIZE;
  }
  //write B back to memory
  la_AsubBmulC(B_mem_REG, B_sch_REG, X_src_sch_REG, One_REG, M);
}


//assumes P == 1, for HPL!
void left_upper_fast(double *A, double *B, IDX *pvec, IDX M, double alpha)
{
  const LaAddr B_SCH_ADR      = 0;
  const LaAddr X_SCH_ADR      = SCRATCH_SIZE/2;

  const LaRegIdx Zero_REG     = 0;
  const LaRegIdx Alpha_REG    = 1;
  const LaRegIdx tmp_REG      = 2;
  const LaRegIdx X_sch_REG    = 3;
  const LaRegIdx B_mem_REG    = 4;
  const LaRegIdx B_sch_REG    = 5;
  const LaRegIdx Ajm_mem_REG  = 6;
  const LaRegIdx Ajj_mem_REG  = 7;

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);
  la_set_scalar_dp_reg(tmp_REG, 0.0); //NOTE: need to set this to 0 right away!

  //precompute alpha*Bjk
  la_set_vec_adr_dp_mem(B_mem_REG, (LaAddr)B);
  la_set_vec_adr_dp_sch(B_sch_REG, B_SCH_ADR);
  la_AmulBaddC(B_sch_REG, Alpha_REG, B_mem_REG, Zero_REG, M);

  //solve each item in B column at a time
  IDX X_adr = X_SCH_ADR+DPSIZE*(M-1);
  IDX B_adr = B_SCH_ADR+DPSIZE*(M-1);
  for(IDX j=M-1; j>=0; --j){
    LaAddr kcount = M-1-j;
    LaAddr Ajm = (LaAddr) &IDXR(A, pvec[j], M-1, M, M);
    LaAddr Ajj = (LaAddr) &IDXR(A, pvec[j], j,   M, M);

    la_set_vec_dp_mem(Ajm_mem_REG, Ajm,                    -1, kcount, 0);
    la_set_vec_dp_sch(X_sch_REG,   X_SCH_ADR+DPSIZE*(M-1), -1, kcount, 0);
    la_AmulBaddC_sum(tmp_REG, X_sch_REG, Ajm_mem_REG, Zero_REG, kcount);

    la_set_scalar_dp_sch(X_sch_REG, X_adr);
    la_set_scalar_dp_sch(B_sch_REG, B_adr);
    la_set_scalar_dp_mem(Ajj_mem_REG, Ajj);
    la_AsubBdivC(X_sch_REG, B_sch_REG, tmp_REG, Ajj_mem_REG, 1);

    X_adr -= DPSIZE;
    B_adr -= DPSIZE;
  }

  //write B back to memory
  la_set_vec_adr_dp_mem(B_mem_REG, (LaAddr)B);
  la_set_vec_adr_dp_sch(X_sch_REG, X_SCH_ADR);
  la_copy(B_mem_REG, X_sch_REG, M);
}





//A row-major and B col-major
void dlu_solve_la_core_fast(double *A, double *B, IDX N)
{
  IDX *pvec = (IDX *)malloc(N*sizeof(IDX));
  for(IDX i=0; i<N; ++i){
    pvec[i] = i;
  }

  if(DEBUG) {
    dump_matrix_dr("A_orig", A, N, N);
    dump_matrix_dc("B_orig", B, N, 1);
  }
  lu_decompose_in_place_fast(A, B, pvec, N);
  left_lower_fast(A, B, pvec, N, 1.0);
  left_upper_fast(A, B, pvec, N, 1.0);

  free(pvec);
}
