//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <gsl/gsl_matrix.h>

#include <cuda_runtime.h>
#include <cuda.h>

extern "C" {
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/common.h"
}

//==========================================================================

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;

static IDX BLOCK_SIZE = 8;

//A trans, B trans
__global__ void Ptrans_kernel(double *dA, double *dB, IDX N)
{
  int i = (blockIdx.x*blockDim.x + threadIdx.x)*BLOCK_SIZE;
  int j = (blockIdx.y*blockDim.y + threadIdx.y)*BLOCK_SIZE;

  //let half the threads just die
  if(j > i) {
    for(IDX ik=0; ik<BLOCK_SIZE; ++ik) {
      for(IDX jk=0; jk<BLOCK_SIZE; ++jk) {
        double tmp = IDXR(dA, j+jk, i+ik, N, N) + IDXR(dB, i+ik, j+jk, N, N);
        IDXR(dA, j+jk, i+ik, N, N) = 
          IDXR(dA, i+ik, j+jk, N, N) + IDXR(dB, j+jk, i+ik, N, N);
        IDXR(dA, i+ik, j+jk, N, N) = tmp;
      }
    }
  }
}

//According to book, limits are:
//  up to 512 threads per block
//  up to 8 blocks per SM at a time
//  up to 1024 threads per SM
//so: blocks should be 16x16 threads (256 threads) (matrices MUST be 16x16 mul)
//    which satisfies all 3 above
static void do_test_cuda(double *A, double *B, IDX N)
{
  IDX BLOCK_SQAR = BLOCK_SIZE*BLOCK_SIZE;
  IDX N_SQAR = N*N;
  IDX MAX_THREADS_PER_BLOCK = 16;

  IDX THREADS_PER_BLOCK = MIN(MAX_THREADS_PER_BLOCK, N/BLOCK_SIZE);
  IDX THREAD_BLOCKS = (N/BLOCK_SIZE)*(N/(BLOCK_SIZE*THREADS_PER_BLOCK));

  double *dC, *dA, *dB;
  IDX C_size = N*M*sizeof(double);
  IDX A_size = N*P*sizeof(double);
  IDX B_size = P*M*sizeof(double);

  //allocate matrices on device
  cudaMalloc((void **)&dA, A_size);
  cudaMalloc((void **)&dB, B_size);

  //transfer A, B, to device
  cudaMemcpy(dA, A, A_size, cudaMemcpyHostToDevice);
  cudaMemcpy(dB, B, B_size, cudaMemcpyHostToDevice);

  //perform kernel
  assert(N % BLOCK == 0);
  dim3 dimGrid(THREAD_BLOCKS, 1);
  dim3 dimBlock(THREADS_PER_BLOCK, 1);
  Ptrans_kernel<<<dimGrid, dimBlock>>>(dA, dB, N);

  //transfer C to host
  cudaMemcpy(A, dA, A_size, cudaMemcpyDeviceToHost);  

  //free matrices on device
  cudaFree(dA);
  cudaFree(dB);
}

static void do_test(IDX N)
{
  double *A, *A_cuda, *B;
  B = create_matrix_dr(N, N, FILL);
  A = create_matrix_dr(N, N, FILL);
  A_cuda = copy_matrix_dr(A, N, N);

  if(DEBUG){
    dump_matrix_dr("CUDA: A before", A_cuda, N, N);
    dump_matrix_dr("GSL: A before", A, N, N);
    dump_matrix_dr("GSL: B before", B, N, N);
  }

//==========================================================================
  double start = get_real_time();
  do_test_cuda(A_cuda, B, N);
  double duration = (get_real_time() - start)*1e6;

  double flop = PTRANS_FLOP(N);
  PRINT("N=%4d | CUDA took % 10.6f secs | ",  N, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration);

//==========================================================================
  start = get_real_time();
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, N, N);
  gsl_matrix_view gsl_B = gsl_matrix_view_array(B, N, N);
  gsl_matrix_transpose (&gsl_A.matrix);
  gsl_matrix_add (&gsl_A.matrix, &gsl_B.matrix);
  duration = (get_real_time() - start)*1e6;

  PRINT("N=%4d | GSL took % 10.6f secs | ",  N, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration);

  for(IDX i=0; i<N; ++i){
    for(IDX j=0; j<N; ++j){
      double num1 = IDXR(A, i, j, N, N);
      double num2 = IDXR(A_cuda, i, j, N, N);
      if(MARGIN_EXCEEDED(num1, num2)){
        printf("Matrices differ at [%d,%d]\n", i, j);
        dump_matrix_dr("CUDA: A after", A_cuda, N, N);
        dump_matrix_dr("GSL: A after", A, N, N);
        return;
      }
    }
  }
  printf("matrix correct\n");

  if(DEBUG){
    dump_matrix_dr("CUDA: A after", A_cuda, N, N);
    dump_matrix_dr("GSL: A after", A, N, N);
  }

  free_matrix_d(A_cuda);
  free_matrix_d(A);
  free_matrix_d(B);
}

int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX nmp_start = NMP_MIN;
  IDX nmp_end = NMP_MAX;

  unsigned tmp_seed;
  IDX tmp_nmp;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(!strcmp(argv[i], "--debug")){
      DEBUG = true;
    }
    else if(sscanf(argv[i], "--log_size=%u", &tmp_nmp) == 1){
      nmp_start = (1 << tmp_nmp);
      nmp_end = (1 << tmp_nmp);
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

  for(IDX NMP=nmp_start; NMP <= nmp_end; NMP *= NMP_SCALE){
    do_test(NMP);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
