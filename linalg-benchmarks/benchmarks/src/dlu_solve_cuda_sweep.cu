//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

extern "C" {
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
}

static const IDX NP_MIN = 4;
static const IDX NP_MAX = 1024;
static const IDX NP_SCALE = 2;

//==========================================================================

//row major order
static void swap_rows_dr(double *A, IDX h, IDX w, IDX r1, IDX r2)
{
  double *tmp = (double *)malloc(sizeof(double)*w);
  double *r1_start = &(IDXR(A, r1, 0, h, w));
  double *r2_start = &(IDXR(A, r2, 0, h, w));
  memcpy(tmp, r1_start, sizeof(double)*w);
  memcpy(r1_start, r2_start, sizeof(double)*w);
  memcpy(r2_start, tmp, sizeof(double)*w);
}

//col major order
static void swap_rows_dc(double *A, IDX h, IDX w, IDX r1, IDX r2)
{
  for (IDX i = 0; i < w; i++) {
    double tmp = IDXC(A, r1, i, h, w);
    IDXC(A, r1, i, h, w) = IDXC(A, r2, i, h, w);
    IDXC(A, r2, i, h, w) = tmp;
  }
}

//A in row-major, B in col-major order
void permute_in_place(double *A, double *B, IDX iter, IDX N, IDX P)
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
    swap_rows_dr(A, N, N, col, max_val_row);
    swap_rows_dc(B, N, P, col, max_val_row);
  }
}

//==========================================================================
// In place LU Factorization
//==========================================================================

__global__ void l_iteration_in_place(double *A, IDX N, IDX iter)
{
  for (IDX j = (i + 1); j<N; ++j) {
    double *Lji = &(IDXR(A, j, i, N, N));
    double Uii = IDXR(A, i, i, N, N);

    for (IDX k; k<i; ++k) {
      double Ljk = IDXR(A, j, k, N, N);
      double Uki = IDXR(A, k, i, N, N);
      *Lji -= Ljk * Uki;
    }
    *Lji /= Uii;
  }
}

__global__ void u_iteration_in_place(double *A, IDX N, IDX iter)
{
  IDX i = iter;
  IDX j = (blockIdx.x*blockDim.x + threadIdx.x) + iter;

  if(iter > 0) {
    double *Uip1j = &(IDXR(A, i + 1, j, N, N));
    for (IDX k; k<i; ++k) {
      double Lip1k =IDXR(A, i + 1, k, N, N);
      double Ukj = IDXR(A, k, j, N, N);
      *Uip1j -= Lip1k * Ukj;
    }
  }
}

static void do_cuda_lu_solve(double *A, double *B, IDX N, IDX P)
{
  const IDX MAX_THREADS = 512;

  double *dC, *dA, *dB;
  IDX C_size = N*M*sizeof(double);
  IDX A_size = N*P*sizeof(double);
  IDX B_size = P*M*sizeof(double);

  //allocate matrices on device
  cudaMalloc((void **)&dC, C_size);
  cudaMalloc((void **)&dA, A_size);
  cudaMalloc((void **)&dB, B_size);

  //transfer A, B, to device
  cudaMemcpy(dA, A, A_size, cudaMemcpyHostToDevice);
  cudaMemcpy(dB, B, B_size, cudaMemcpyHostToDevice);
  cudaMemcpy(dC, C, C_size, cudaMemcpyHostToDevice);

  //perform kernel
  for (IDX iter = 0; iter < N; ++iter) {
    permute_in_place(A, B, iter, N, P);

    IDX num_threads = MIN(size/2, MAX_THREADS);
    dim3 dimGrid(M/BLOCK_SIZE, 1);
    dim3 dimBlock(BLOCK_SIZE, 1);
    u_iteration_in_place<<<dimGrid,dimBlock>>>(A, N, iter);

    l_iteration_in_place<<<dimGrid,dimBlock>>>(A, N, iter);
  }
  //use_cublas(dC, dA, dB, N, M, P, A_trans, B_trans);

  //transfer C to host
  cudaMemcpy(C, dC, C_size, cudaMemcpyDeviceToHost);  

  //free matrices on device
  cudaFree(dC);
  cudaFree(dA);
  cudaFree(dB);
}

static void do_test(IDX N, IDX P)
{
  double *A, *B;
  A = create_matrix_dr(N, N, FILL);
  B = create_matrix_dc(N, P, FILL);

  double start = get_real_time();
  do_cuda_lu_solve(A, B, N, P);
  double duration = get_real_time() - start;

  double flop = LU_SOLVE_FLOP(N);
  PRINT("N=%4d P=%4d | took % 10.6f secs | ", N, P, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration / 1e6);

  free_matrix_d(A);
  free_matrix_d(B);
}

int main(int argc, char **argv)
{
  //-------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX np_start  = NP_MIN;
  IDX np_end    = NP_MAX;

  unsigned tmp_seed;
  IDX tmp_log_size;

  //-------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(sscanf(argv[i], "--log_size=%u", &tmp_log_size) == 1){
      np_start = (1 << tmp_log_size);
      np_end = (1 << tmp_log_size);
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //-------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //-------------------------------------------------------------------------

  //set breakpoint here, then make command window huge
  for(IDX NP=np_start; NP <= np_end; NP *= NP_SCALE){
    do_test(NP, NP);
  }

  //set breakpoint here, then analyze results
  return 0;
}
