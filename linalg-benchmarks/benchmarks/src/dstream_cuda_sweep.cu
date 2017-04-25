//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// CUDA implementation of the HPCC STREAM benchmark
// http://www.cs.virginia.edu/~mccalpin/papers/bandwidth/node2.html
// http://www.cs.virginia.edu/stream/ref.html
//
// For bandwidth, using the 2st of the 3 methods, counting reads and writes 
// as separate transfers.
//=========================================================================

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

extern "C" {
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
}

//==========================================================================

static const IDX VEC_MIN       = (1 << 16);
static const IDX VEC_MAX       = (1 << 20);
static const IDX VEC_SCALE     = 2;

static const double q          = 2.5;

//==========================================================================
// COPY
//==========================================================================

void do_copy(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A = create_matrix_dr(size, 1, NO_FILL);
  double *B = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  for(IDX i=0; i<size; i+=1) {
    A[i] = B[i];
  }
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_COPY_FLOP(size);
  double bytes = STREAM_COPY_DP_BYTES(size);
  printf("COPY  SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
}

//==========================================================================
// SCALE
//==========================================================================

__global__ void DoScaleKernel(double *dA, double *dB, IDX count)
{
  int start = blockIdx.x*blockDim.x + threadIdx.x;
  int end = start + count;

  for (IDX i=start; i<end; ++i) {
    dA[i] = q*dB[i];
  }
}

//minimum of 4096 elements
void do_cuda_scale(double *A, double *B, IDX size)
{
  const IDX BLOCK_SIZE = 256;
  const IDX IDX_PER_THREAD = MIN(size/BLOCK_SIZE, 16);

  double *dA, *dB;
  IDX A_size = size*sizeof(double);
  IDX B_size = size*sizeof(double);

  //allocate matrices on device
  cudaMalloc((void **)&dA, A_size);
  cudaMalloc((void **)&dB, B_size);

  //transfer A, to device
  cudaMemcpy(dB, B, B_size, cudaMemcpyHostToDevice);

  //perform kernel
  assert(size % BLOCK_SIZE*IDX_PER_THREAD == 0);
  dim3 dimGrid(size/BLOCK_SIZE/IDX_PER_THREAD, 1);
  dim3 dimBlock(BLOCK_SIZE, 1);
  DoScaleKernel<<<dimGrid, dimBlock>>>(dA, dB, IDX_PER_THREAD);

  //transfer A to host
  cudaMemcpy(A, dA, A_size, cudaMemcpyDeviceToHost);  

  //free matrices on device
  cudaFree(dA);
  cudaFree(dB);
}

//According to book, limits are:
//  up to 512 threads per block
//  up to 8 blocks per SM at a time
//  up to 1024 threads per SM
//so: blocks should be 256x1 threads (256 threads) which satisfies all 3 above
void do_scale(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A = create_matrix_dr(size, 1, NO_FILL);
  double *B = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  do_cuda_scale(A, B, size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_SCALE_FLOP(size);
  double bytes = STREAM_SCALE_DP_BYTES(size);
  printf("SCALE SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
}

//==========================================================================
// SUM
//==========================================================================

__global__ void DoSumKernel(double *dA, double *dB, double *dC, IDX count)
{
  int start = blockIdx.x*blockDim.x + threadIdx.x;
  int end = start + count;

  for (IDX i=start; i<end; ++i) {
    dA[i] = dB[i] + dC[i];
  }
}

//minimum of 4096 elements
void do_cuda_sum(double *A, double *B, double *C, IDX size)
{
  const IDX BLOCK_SIZE = 256;
  const IDX IDX_PER_THREAD = MIN(size/BLOCK_SIZE, 16);

  double *dA, *dB, *dC;
  IDX A_size = size*sizeof(double);
  IDX B_size = size*sizeof(double);
  IDX C_size = size*sizeof(double);

  //allocate matrices on device
  cudaMalloc((void **)&dA, A_size);
  cudaMalloc((void **)&dB, B_size);
  cudaMalloc((void **)&dC, C_size);

  //transfer B, C, to device
  cudaMemcpy(dB, B, B_size, cudaMemcpyHostToDevice);
  cudaMemcpy(dC, C, C_size, cudaMemcpyHostToDevice);

  //perform kernel
  assert(size % BLOCK_SIZE*IDX_PER_THREAD == 0);
  dim3 dimGrid(size/BLOCK_SIZE/IDX_PER_THREAD, 1);
  dim3 dimBlock(BLOCK_SIZE, 1);
  DoSumKernel<<<dimGrid, dimBlock>>>(dA, dB, dC, IDX_PER_THREAD);

  //transfer A to host
  cudaMemcpy(A, dA, A_size, cudaMemcpyDeviceToHost);   

  //free matrices on device
  cudaFree(dA);
  cudaFree(dB);
  cudaFree(dC);
}

//According to book, limits are:
//  up to 512 threads per block
//  up to 8 blocks per SM at a time
//  up to 1024 threads per SM
//so: blocks should be 256x1 threads (256 threads) which satisfies all 3 above
void do_sum(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A         = create_matrix_dr(size, 1, NO_FILL);
  double *B         = create_matrix_dr(size, 1, FILL);
  double *C         = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  do_cuda_sum(A, B, C, size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_SUM_FLOP(size);
  double bytes = STREAM_SUM_DP_BYTES(size);
  printf("SUM   SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(C);
}

//==========================================================================
// TRIAD
//==========================================================================

__global__ void DoTriadKernel(double *dA, double *dB, double *dC, IDX count)
{
  int start = blockIdx.x*blockDim.x + threadIdx.x;
  int end = start + count;

  for (IDX i=start; i<end; ++i) {
    dA[i] = dB[i] + q*dC[i];
  }
}

//minimum of 4096 elements
void do_cuda_triad(double *A, double *B, double *C, IDX size)
{
  const IDX BLOCK_SIZE = 256;
  const IDX IDX_PER_THREAD = MIN(size/BLOCK_SIZE, 16);

  double *dA, *dB, *dC;
  IDX A_size = size*sizeof(double);
  IDX B_size = size*sizeof(double);
  IDX C_size = size*sizeof(double);

  //allocate matrices on device
  cudaMalloc((void **)&dA, A_size);
  cudaMalloc((void **)&dB, B_size);
  cudaMalloc((void **)&dC, C_size);

  //transfer B, C, to device
  cudaMemcpy(dB, B, B_size, cudaMemcpyHostToDevice);
  cudaMemcpy(dC, C, C_size, cudaMemcpyHostToDevice);

  //perform kernel
  assert(size % BLOCK_SIZE*IDX_PER_THREAD == 0);
  dim3 dimGrid(size/BLOCK_SIZE/IDX_PER_THREAD, 1);
  dim3 dimBlock(BLOCK_SIZE, 1);
  DoTriadKernel<<<dimGrid, dimBlock>>>(dA, dB, dC, IDX_PER_THREAD);

  //transfer A to host
  cudaMemcpy(A, dA, A_size, cudaMemcpyDeviceToHost);   

  //free matrices on device
  cudaFree(dA);
  cudaFree(dB);
  cudaFree(dC);
}

void do_triad(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A         = create_matrix_dr(size, 1, NO_FILL);
  double *B         = create_matrix_dr(size, 1, FILL);
  double *C         = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  do_cuda_triad(A, B, C, size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_TRIAD_FLOP(size);
  double bytes = STREAM_TRIAD_DP_BYTES(size);
  printf("TRIAD SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(C);
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
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(sscanf(argv[i], "--log_size=%u", &tmp_log_size) == 1){
      size_start = (1 << tmp_log_size);
      size_end = (1 << tmp_log_size);
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
  for(IDX SIZE=size_start; SIZE <= size_end; SIZE *= VEC_SCALE){
    do_copy(SIZE);
    do_scale(SIZE);
    do_sum(SIZE);
    do_triad(SIZE);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}

