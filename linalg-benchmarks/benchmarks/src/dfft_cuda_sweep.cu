//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// x86 implementation of the HPCC FFT benchmark
// Naive Algorithm pretty much straight from CLRS Algorithms textbook
//=========================================================================

#include <assert.h>
#include <cuda.h>
#include <math.h>
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
// Definitions of global control flags
bool DEBUG = false;

#define VEC_MIN 8
#define VEC_MAX 10
#define VEC_SCALE 2


//==========================================================================

// Block index
#define bx blockIdx.x
#define by blockIdx.y
// Thread index
#define tx threadIdx.x

// Possible values are 2, 4, 8 and 16
//#define R 16
#define R 16

inline double2 operator*(double2 a, double2 b)
{
    return make_double2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

inline double2 operator+(double2 a, double2 b)
{
    return make_double2(a.x + b.x, a.y + b.y);
}

inline double2 operator-(double2 a, double2 b)
{
    return make_double2(a.x - b.x, a.y - b.y);
}

inline double2 operator*(double2 a, double b)
{
    return make_double2(b * a.x , b * a.y);
}

#define COS_PI_8  0.923879533
#define SIN_PI_8  0.382683432
#define exp_1_16  make_double2( COS_PI_8, -SIN_PI_8)
#define exp_3_16  make_double2( SIN_PI_8, -COS_PI_8)
#define exp_5_16  make_double2(-SIN_PI_8, -COS_PI_8)
#define exp_7_16  make_double2(-COS_PI_8, -SIN_PI_8)
#define exp_9_16  make_double2(-COS_PI_8,  SIN_PI_8)
#define exp_1_8   make_double2( 1, -1)
#define exp_1_4   make_double2( 0, -1)
#define exp_3_8   make_double2(-1, -1)

__device__ void GPU_FFT2(double2 &v1, double2 &v2)
{
    double2 v0 = v1;
    v1 = v0 + v2;
    v2 = v0 - v2;
}

__device__ void GPU_FFT4(double2 &v0, double2 &v1, double2 &v2, double2 &v3)
{
    GPU_FFT2(v0, v2);
    GPU_FFT2(v1, v3);
    v3 = v3 * exp_1_4;
    GPU_FFT2(v0, v1);
    GPU_FFT2(v2, v3);
}

inline __device__ void GPU_FFT2(double2* v)
{
    GPU_FFT2(v[0], v[1]);
}

inline __device__ void GPU_FFT4(double2* v)
{
    GPU_FFT4(v[0], v[1], v[2], v[3]);
}

inline __device__ void GPU_FFT8(double2* v)
{
    GPU_FFT2(v[0], v[4]);
    GPU_FFT2(v[1], v[5]);
    GPU_FFT2(v[2], v[6]);
    GPU_FFT2(v[3], v[7]);

    v[5] = (v[5] * exp_1_8) * M_SQRT1_2;
    v[6] = v[6] * exp_1_4;
    v[7] = (v[7] * exp_3_8) * M_SQRT1_2;

    GPU_FFT4(v[0], v[1], v[2], v[3]);
    GPU_FFT4(v[4], v[5], v[6], v[7]);

}

inline __device__ void GPU_FFT16(double2 *v)
{
    GPU_FFT4(v[0], v[4], v[8], v[12]);
    GPU_FFT4(v[1], v[5], v[9], v[13]);
    GPU_FFT4(v[2], v[6], v[10], v[14]);
    GPU_FFT4(v[3], v[7], v[11], v[15]);

    v[5]  = (v[5]  * exp_1_8) * M_SQRT1_2;
    v[6]  =  v[6]  * exp_1_4;
    v[7]  = (v[7]  * exp_3_8) * M_SQRT1_2;
    v[9]  =  v[9]  * exp_1_16;
    v[10] = (v[10] * exp_1_8) * M_SQRT1_2;
    v[11] =  v[11] * exp_3_16;
    v[13] =  v[13] * exp_3_16;
    v[14] = (v[14] * exp_3_8) * M_SQRT1_2;
    v[15] =  v[15] * exp_9_16;

    GPU_FFT4(v[0],  v[1],  v[2],  v[3]);
    GPU_FFT4(v[4],  v[5],  v[6],  v[7]);
    GPU_FFT4(v[8],  v[9],  v[10], v[11]);
    GPU_FFT4(v[12], v[13], v[14], v[15]);
}

__device__ int GPU_expand(int idxL, int N1, int N2)
{
    return (idxL / N1) * N1 * N2 + (idxL % N1);
}

__device__ void GPU_FftIteration(int j, int Ns, double2* data0, double2* data1, int N)
{
    double2 v[R];
    int idxS = j;
    double angle = -2.0 * M_PI * (j % Ns) / (Ns * R);

    for (int r = 0; r < R; r++) {
        v[r] = data0[idxS + r * N / R];
        v[r] = v[r] * make_double2(cos(r * angle), sin(r * angle));
    }

#if R == 2
    GPU_FFT2(v);
#endif

#if R == 4
    GPU_FFT4(v);
#endif

#if R == 8
    GPU_FFT8(v);
#endif

#if R == 16
    GPU_FFT16(v);
#endif

    int idxD = GPU_expand(j, Ns, R);

    for (int r = 0; r < R; r++) {
        data1[idxD + r * Ns] = v[r];
    }

}

__global__ void GPU_FFT_Global(int Ns, double2* data0, double2* data1, int N)
{
    data0 += bx * N;
    data1 += bx * N;
    GPU_FftIteration(tx, Ns, data0, data1, N);
}



//array must be > 512
void do_cuda_fft(double2 *source, double2 *result, IDX size)
{
  IDX MAX_THREADS_PER_BLOCK = 8;
  IDX MIN_THREADS_PER_BLOCK = 8;
  IDX B = MAX(size/R/MAX_THREADS_PER_BLOCK, 1);
  IDX N = size/B;
  if(N/R < MIN_THREADS_PER_BLOCK) {
    N = MIN_THREADS_PER_BLOCK*R;
    B = size/N;
  }
  printf("size,N,R,B = {%d,%d,%d,%d}\n", size, N, R, B);
  assert(B*N == size);

  IDX n_bytes = N * B * sizeof(double2);

   // allocate device memory
  double2 *d_source, *d_work;
  cudaMalloc((void**) &d_source, n_bytes);

  // copy host memory to device
  cudaMemcpy(d_source, source, n_bytes, cudaMemcpyHostToDevice);
  cudaMalloc((void**) &d_work, n_bytes);
  cudaMemset(d_work, 0, n_bytes);

  for (int Ns = 1; Ns < N; Ns *= R) {
    GPU_FFT_Global <<<dim3(B), dim3(N / R)>>> (Ns, d_source, d_work, N);
    double2 *tmp = d_source;
    d_source = d_work;
    d_work = tmp;

    if(DEBUG){
      printf("d_source=[\n  ");
      for(IDX i=0; i<size;++i){
        printf("{%6.3f,%6.3f},", source[i].x, source[i].y);
        if(i%4 == 3){
          printf("\n  ");
        }
      }
      printf("]\n");
    }
  }
  cudaThreadSynchronize();

  // copy device memory to host
  cudaMemcpy(result, d_source, n_bytes, cudaMemcpyDeviceToHost);

  //cudaFree(d_source);
  //cudaFree(d_work);
}

unsigned int my_log2( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}

#define FFT_FLOP_GOOD(size) ((double)(5*size*(my_log2(size))))

void do_fft(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double2 *source = (double2 *)create_matrix_dr(size*2, 1, FILL);
  double2 *result = (double2 *)create_matrix_dr(size*2, 1, NO_FILL);

  for(IDX i=0; i<size; ++i){
    source[i].y = 0.0;
  }

  if(DEBUG){
    printf("source=[\n  ");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", source[i].x, source[i].y);
      if(i%4 == 3){
        printf("\n  ");
      }
    }
    printf("]\n");
  }

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  do_cuda_fft(source, result, size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = FFT_FLOP_GOOD(size);
  printf("SIZE=%8d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  if(DEBUG){
    printf("result=[\n  ");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", result[i].x, result[i].y);
      if(i%4 == 3){
        printf("\n  ");
      }
    }
    printf("]\n");
  }

  //--------------------------- CLEANUP ----------------------------------
  //free_matrix_d((double *)source);
  //free_matrix_d((double *)result);
}



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
    else if(!strcmp(argv[i], "--debug")){
      DEBUG = true;
    }
    else if(sscanf(argv[i], "--log_size=%u", &tmp_log_size) == 1){
      size_start = 1 << tmp_log_size;
      size_end = 1 << tmp_log_size;
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
    do_fft(SIZE);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}

