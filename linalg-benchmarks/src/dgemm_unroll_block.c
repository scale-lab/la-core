//=========================================================================
//naive dgemm c file
//
//Results:
// gsl runs at about ~3.5 GFLOP/s on my machine regardless of input sizes
// my worst results are ~280 MFLOP/s with block-size of 4/unrolling 4
// my best results are: ~820 MFLOP/s: block-size of 64, unroll 8
// 
//=========================================================================

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "get_real_time.h"
#include "gsl_result_check.h"
#include "matrix_utilities.h"

#define BLOCK_MIN 16
#define BLOCK_MAX 128
#define BLOCK_SCALE 2

#define NMP_MIN 16
#define NMP_MAX 2048
#define NMP_SCALE 2


#define UNROLL 16

//NOTE: using col-major order, with A transposed
// C[n][m] = A[n][p] * B[p][m]
void dgemm_block(double *A, double *B, double *C, 
  uint16_t N, uint16_t M, uint16_t P, uint16_t BS, 
  uint16_t i, uint16_t j, uint16_t k)
{
  uint16_t min_i = i;
  uint16_t min_j = j;
  uint16_t min_k = k;
  uint16_t max_i = (i + BS > N) ? N : (i + BS);  //N!
  uint16_t max_j = (j + BS > M) ? M : (j + BS);  //M!
  uint16_t max_k = (k + BS > P) ? P : (k + BS);

  //zero result array
  if (min_k == 0) {
    for (j = min_j; j < max_j; ++j) {
      for (i = min_i; i < max_i; i+=UNROLL) {
        IDXC(C, i+0,  j, N, M) = 0;
        IDXC(C, i+1,  j, N, M) = 0;
        IDXC(C, i+2,  j, N, M) = 0;
        IDXC(C, i+3,  j, N, M) = 0;
        IDXC(C, i+4,  j, N, M) = 0;
        IDXC(C, i+5,  j, N, M) = 0;
        IDXC(C, i+6,  j, N, M) = 0;
        IDXC(C, i+7,  j, N, M) = 0;
        IDXC(C, i+8,  j, N, M) = 0;
        IDXC(C, i+9,  j, N, M) = 0;
        IDXC(C, i+10, j, N, M) = 0;
        IDXC(C, i+11, j, N, M) = 0;
        IDXC(C, i+12, j, N, M) = 0;
        IDXC(C, i+13, j, N, M) = 0;
        IDXC(C, i+14, j, N, M) = 0;
        IDXC(C, i+15, j, N, M) = 0;
      }
    }
  }

  //compute result
  for(j=min_j; j<max_j; ++j){
    for(i=min_i; i<max_i; ++i){
      for(k=min_k; k<max_k; k+=UNROLL){
        IDXC(C, i, j, N, M) += (
          IDXR(A, i, k+0, N, P)*IDXC(B,  k+0,  j, P, M) +
          IDXR(A, i, k+1, N, P)*IDXC(B,  k+1,  j, P, M) +
          IDXR(A, i, k+2, N, P)*IDXC(B,  k+2,  j, P, M) +
          IDXR(A, i, k+3, N, P)*IDXC(B,  k+3,  j, P, M) +
          IDXR(A, i, k+4, N, P)*IDXC(B,  k+4,  j, P, M) +
          IDXR(A, i, k+5, N, P)*IDXC(B,  k+5,  j, P, M) +
          IDXR(A, i, k+6, N, P)*IDXC(B,  k+6,  j, P, M) +
          IDXR(A, i, k+7, N, P)*IDXC(B,  k+7,  j, P, M) +
          IDXR(A, i, k+8, N, P)*IDXC(B,  k+8,  j, P, M) +
          IDXR(A, i, k+9, N, P)*IDXC(B,  k+9,  j, P, M) +
          IDXR(A, i, k+10, N, P)*IDXC(B, k+10, j, P, M) +
          IDXR(A, i, k+11, N, P)*IDXC(B, k+11, j, P, M) +
          IDXR(A, i, k+12, N, P)*IDXC(B, k+12, j, P, M) +
          IDXR(A, i, k+13, N, P)*IDXC(B, k+13, j, P, M) +
          IDXR(A, i, k+14, N, P)*IDXC(B, k+14, j, P, M) +
          IDXR(A, i, k+15, N, P)*IDXC(B, k+15, j, P, M)
        );
      }
    }
  }
}

// C[n][m] = A[n][p] * B[p][m]
void dgemm(double *A, double *B, double *C, 
  uint16_t N, uint16_t M, uint16_t P, uint16_t BS)
{
  double *A_trans = transpose_matrix_dcr(A, N, P);

  for (uint16_t j=0; j<M; j += BS) {
    for (uint16_t i=0; i<N; i += BS) {
      for (uint16_t k=0; k<P; k += BS) {
        dgemm_block(A_trans, B, C, N, M, P, BS, i, j, k);
      }
    }
  }
  destroy_matrix_d(A_trans);
}

void do_test(uint16_t N, uint16_t M, uint16_t P, uint16_t BS)
{
  double *A, *B, *C, *gsl_A, *gsl_B, *gsl_C;

  A = create_matrix_dc(N, P, FILL);
  B = create_matrix_dc(P, M, FILL);
  C = create_matrix_dc(N, M, NO_FILL);

  double start = getRealTime();
  dgemm(A, B, C, N, M, P, BS);
  double duration = getRealTime() - start;

  double flop = (double)((2 * (uint64_t)P - 1)*(uint64_t)N*(uint64_t)M);
  printf("N=%4u M=%4u P=%4u BS=%4u | took % 10.6f secs | ", N, M, P, 
    BS, duration);
  printf("% 10.3f MFLOP | ", flop/1e6);
  printf("% 10.3f MFLOP/s || ", flop/(duration*1e6));

  gsl_A = transpose_matrix_dcr(A, N, P);
  gsl_B = transpose_matrix_dcr(B, P, M);
  gsl_C = transpose_matrix_dcr(C, N, M);
  verify_gsl_dgemm(gsl_A, gsl_B, gsl_C, N, M, P);

  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(C);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
  destroy_matrix_d(gsl_C);
}

//NOTE: I confirmed by walking back down the block size that the performane
//on the way up and down for each block size was the same. This was to make
//sure the caches weren't getting 'warmed up' over each iteration. It was 
//verified that no 'warming up' was happening. so i just commented those 
//lines out to save runtime
int main(int argc, char **argv)
{
  //set breakpoint here, then make command window huge
  srand((unsigned)time(NULL));

  for(uint16_t NMP=NMP_MIN; NMP <= NMP_MAX; NMP *= NMP_SCALE){
    for(uint16_t BS=BLOCK_MIN; BS<=BLOCK_MAX; BS *= BLOCK_SCALE){
      do_test(NMP, NMP, NMP, BS);
    }
    printf("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}