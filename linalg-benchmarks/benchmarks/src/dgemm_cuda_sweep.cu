//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

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

static const double alpha       = 2.5;
static const double beta        = 3.0;

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;

static const bool YES_TRANS     = 1;
static const bool NO_TRANS      = 0;


//A not trans, B not trans
__global__ void MatrixMulKernel_NN(double *dC, double *dA, double *dB,
  IDX N, IDX M, IDX P)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  int j = blockIdx.y*blockDim.y + threadIdx.y;

  double result = 0;
  for (IDX k=0; k<P; ++k) {
    result += IDXR(dA, i, k, N, P)*IDXR(dB, k, j, P, M);
  }
  IDXR(dC, i, j, N, M) = alpha*result + beta*IDXR(dC, i, j, N, M); 
}

//A not trans, B trans
__global__ void MatrixMulKernel_NT(double *dC, double *dA, double *dB,
  IDX N, IDX M, IDX P)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  int j = blockIdx.y*blockDim.y + threadIdx.y;

  double result = 0;
  for (IDX k=0; k<P; ++k) {
    result += IDXR(dA, i, k, N, P)*IDXR(dB, j, k, P, M);
  }
  IDXR(dC, i, j, N, M) = alpha*result + beta*IDXR(dC, i, j, N, M); 
}

//A trans, B not trans
__global__ void MatrixMulKernel_TN(double *dC, double *dA, double *dB,
  IDX N, IDX M, IDX P)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  int j = blockIdx.y*blockDim.y + threadIdx.y;

  double result = 0;
  for (IDX k=0; k<P; ++k) {
    result += IDXR(dA, k, i, N, P)*IDXR(dB, k, j, P, M);
  }
  IDXR(dC, i, j, N, M) = alpha*result + beta*IDXR(dC, i, j, N, M); 
}

//A trans, B trans
__global__ void MatrixMulKernel_TT(double *dC, double *dA, double *dB,
  IDX N, IDX M, IDX P)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  int j = blockIdx.y*blockDim.y + threadIdx.y;

  double result = 0;
  for (IDX k=0; k<P; ++k) {
    result += IDXR(dA, k, i, N, P)*IDXR(dB, j, k, P, M);
  }
  IDXR(dC, i, j, N, M) = alpha*result + beta*IDXR(dC, i, j, N, M); 
}


//  NO STATIC LINKING IN CUDA 3.2 :(
/*
void use_cublas(double *dC, double *dA, double *dB,
  IDX N, IDX M, IDX P, bool A_trans, bool B_trans)
{
  cublasDgemm((A_trans ? 'T' : 'N' ),
    (B_trans ? 'T' : 'N'), N, M, P, alpha,
    dA, N, dB, P, beta, dC, N);
}
*/

//According to book, limits are:
//  up to 512 threads per block
//  up to 8 blocks per SM at a time
//  up to 1024 threads per SM
//so: blocks should be 16x16 threads (256 threads) (matrices MUST be 16x16 mul)
//    which satisfies all 3 above
static void do_cuda_dgemm(double *C, double *A, double *B,
  IDX N, IDX M, IDX P, bool A_trans, bool B_trans)
{
  const IDX BLOCK_SIZE = 16;

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
  assert(N % BLOCK_SIZE == 0 && M % BLOCK_SIZE == 0);
  dim3 dimGrid(M/BLOCK_SIZE, N/BLOCK_SIZE);
  dim3 dimBlock(BLOCK_SIZE, BLOCK_SIZE);
  if(!A_trans && !B_trans) {
    MatrixMulKernel_NN<<<dimGrid, dimBlock>>>(dC, dA, dB, N, M, P);
  } else if(!A_trans && B_trans) {
    MatrixMulKernel_NT<<<dimGrid, dimBlock>>>(dC, dA, dB, N, M, P);
  } else if(A_trans && !B_trans) {
    MatrixMulKernel_TN<<<dimGrid, dimBlock>>>(dC, dA, dB, N, M, P);
  } else {
    MatrixMulKernel_TT<<<dimGrid, dimBlock>>>(dC, dA, dB, N, M, P);
  }
  //use_cublas(dC, dA, dB, N, M, P, A_trans, B_trans);

  //transfer C to host
  cudaMemcpy(C, dC, C_size, cudaMemcpyDeviceToHost);  

  //free matrices on device
  cudaFree(dC);
  cudaFree(dA);
  cudaFree(dB);
}


//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
/*
    -- MAGMA (version 0.3) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       June 2010

    -- Developed by:
       Rajib Nath 
       Stan Tomov

    -- The implementation of these DGEMM routines for the Fermi GPUs are described in
       Nath, R., Tomov, S., Dongarra, J. "An Improved MAGMA GEMM for Fermi GPUs," 
       University of Tennessee Computer Science Technical Report, UT-CS-10-655 
       (also LAPACK working note 227), July 29, 2010. 

       These routines will be included in CUBLAS 3.2.
*/

/*
    blk_M=64 blk_N=64 blk_K=16 nthd_x=64 nthd_y=4
*/

//texture<int2,1>  tex_x_double_A;
//texture<int2,1>  tex_x_double_B;

static __inline__ __device__ double fetch_x_A(const int& i, const double *A)
{
  return A[i];
  //register int2  v = tex1Dfetch(tex_x_double_A, i);
  //return __hiloint2double(v.y, v.x);
}
static __inline__ __device__ double fetch_x_B(const int& i, const double *B)
{
  return B[i];
  //register int2  v = tex1Dfetch(tex_x_double_B, i);
  //return __hiloint2double(v.y, v.x);
}


__global__ void fermiDgemm_v2_kernel_NN(double *C, const double *A, const double *B,  
                                        int m, int n, int k, int lda, int ldb,  
                                        int ldc, double alpha, double beta) 
{
    const  int tx = threadIdx.x;
    const  int ty = threadIdx.y;

    const int iby = blockIdx.y * 64;
    const int ibx = blockIdx.x * 64;
    const int idt = ty * 64 + tx;

    const int res = idt%16;
    const int qot = idt/16;

    __shared__ double  Bb[16][65];
    __shared__ double Abs[64][17];

    double xxA[4];
    double xxB[4];
    
    B+= res+ __mul24(iby + qot * 4, ldb );
    int trackB = res+ __mul24(iby + qot * 4, ldb );

    A += ibx +__mul24( qot, lda) + res ; 
    int trackA =  ibx +__mul24( qot, lda) + res ;

    #pragma unroll
    for(int y=0; y<4; y++)
       Bb[res][qot*4+y] = fetch_x_B( trackB + y * ldb, B ) ;

    #pragma unroll
    for(int y=0; y<4; y++)
      Abs[res+ y*16][qot] = fetch_x_A(trackA + y*16, A) ;
    __syncthreads();

    const double *Bend = B + k-16;
   
    double Axs[4];
    double Bxp[4];

    double Cb[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

     do 
     {
    B += 16;
    A += lda *16  ;
    trackA += 16*lda ; 
    trackB += 16;

        #pragma unroll
        for( int y=0; y<4; y++)
      xxB[y] = fetch_x_B( trackB + y*ldb, B);

        #pragma unroll
        for( int y=0; y<4; y++)
      xxA[y] = fetch_x_A(trackA + y*16, A);

        #pragma unroll 
        for( int j1=0;j1<16;j1++){

             #pragma unroll
             for( int y=0; y<4; y++)
                  Bxp[y]= Bb[j1][qot+y*16];

             #pragma unroll
             for( int y=0; y<4; y++)
                  Axs[y] =  Abs[res+y*16][j1] ;

             #pragma unroll 
             for( int x=0; x<4; x++){
                  #pragma unroll 
                  for( int y=0; y<4; y++){
                      Cb[x*4+y]  += Axs[x]*Bxp[y];
          }
         }
        }

    __syncthreads();
    #pragma unroll
    for(int y=0; y<4; y++)
        Abs[res+y*16][qot] = xxA[y]; 

    #pragma unroll
    for(int y=0; y<4; y++)
        Bb[res][qot*4 + y] = xxB[y];

    __syncthreads();
     }
     while (B < Bend);

     C += res + ibx  + __mul24 (qot +  iby ,ldc);

     #pragma unroll 
     for(int j1=0;j1<16;j1++){

              #pragma unroll
              for( int y=0; y<4; y++)
                  Bxp[y]= Bb[j1][qot + y*16];

              #pragma unroll
              for( int y=0; y<4; y++)
                  Axs[y] =  Abs[res + y*16][j1] ;

              #pragma unroll 
              for( int x=0; x<4; x++){
                  #pragma unroll 
                  for( int y=0;y<4; y++){
                      Cb[x*4 + y]  += Axs[x]*Bxp[y];
          }
          }
    }

    #pragma unroll
    for( int y=0;y<4;y++){

       #pragma unroll
       for(int x=0;x<4;x++){
          C[x*16] = alpha*Cb[y+x*4] + beta * C[x*16];
       }
       
       C += ldc*16;
    }
}

__global__ void fermiDgemm_v2_kernel_TN(double *C, const double *A, const double *B,
                                        int m, int n,  int k,  int lda,  int ldb,  
                                        int ldc, double alpha, double beta) 
{
    const  int tx = threadIdx.x;
    const  int ty = threadIdx.y;

    const int iby = blockIdx.y * 64;
    const int ibx = blockIdx.x * 64;
    const int idt = ty * 64 + tx;

    const int res = idt%16;
    const int qot = idt/16;

    __shared__ double Bb[16][65];
    __shared__ double Abs[64][17];

    double xxA[4];
    double xxB[4];

    B+= res+ __mul24(iby+qot*4, ldb );
    int trackB = res+ __mul24(iby+qot*4, ldb );

    A+= __mul24( ibx + qot, lda )   + res; 
    int trackA = __mul24( ibx + qot, lda ) + res;

    #pragma unroll
    for(int y=0; y<4; y++)
    Bb[res][qot*4+y] =fetch_x_B( trackB + y*ldb, B );

    #pragma unroll
    for(int y=0; y<4; y++)
    Abs[qot+16*y][res] = fetch_x_A(trackA +  lda*16*y, A);
    __syncthreads();

    const double *Bend = B + k-16;
   
    double Axs[4];
    double Bxp[4];

    double Cb[16] = {0,0,0,0,    0,0,0,0, 0,0,0,0, 0,0,0,0};

    do 
    {
    B += 16;
    A += 16  ;
    trackA+=16 ; 
    trackB+=16;

        #pragma unroll
        for(int y=0; y<4; y++)
       xxB[y]=fetch_x_B( trackB + y*ldb, B);

    #pragma unroll
        for(int y=0; y<4; y++)
       xxA[y] =fetch_x_A(trackA +  lda*y*16, A);

    #pragma unroll 
        for(int j1=0;j1<16;j1++){
                  #pragma unroll
                  for(int y=0; y<4; y++)
                      Bxp[y]= Bb[j1][qot+y*16];

                  #pragma unroll
                  for(int y=0; y<4; y++)
                      Axs[y] =  Abs[res+y*16][j1];

                  #pragma unroll 
                  for(int x=0; x<4; x++){
                     #pragma unroll 
                     for(int y=0; y<4; y++){
                         Cb[x*4+y]  += Axs[x]*Bxp[y];
             }
          }
    }

    __syncthreads();
    #pragma unroll
    for(int y=0; y<4; y++)
       Abs[qot+16*y][res] = xxA[y];
 
    #pragma unroll
    for(int y=0; y<4; y++)
       Bb[res][qot*4+y] =xxB[y];
    __syncthreads();
    } 
    while (B < Bend);

    C += res + ibx  + __mul24 (qot + iby ,ldc);

    #pragma unroll 
    for(int j1=0; j1<16; j1++){
    #pragma unroll
        for(int y=0; y<4; y++)
            Bxp[y]= Bb[j1][qot+y*16];

        #pragma unroll
        for(int y=0; y<4; y++)
            Axs[y] = Abs[res+y*16][j1];

        #pragma unroll 
        for(int x=0; x<4; x++){
            #pragma unroll 
            for(int y=0; y<4; y++){
               Cb[x*4+y] += Axs[x]*Bxp[y];
        }
    }
    }

    #pragma unroll
    for(int y=0;y<4;y++){
    #pragma unroll
        for(int x=0;x<4;x++)
       C[x*16] =alpha*Cb[y+x*4] + beta * C[x*16];
       
    C+=ldc*16;
    }
}


__global__ 
void fermiDgemm_v2_kernel_TT(double *C, const double *A, const double *B,  int m, int n,  
                             int k,  int lda,  int ldb,  int ldc, double alpha, double beta) 
{
    const  int tx = threadIdx.x;
    const  int ty = threadIdx.y;

    const int iby = blockIdx.y * 64;
    const int ibx = blockIdx.x * 64;
    const int idt = ty * 64 + tx;

    const int res = idt% 16;
    const int qot = idt/ 16;

    __shared__ double Bb[16][65];
    __shared__ double Abs[64][17];

    double xxA[4];
    double xxB[4];

    B += iby + tx + __mul24(ty , ldb );
    A += __mul24( ibx + qot , lda )   + res; 

    int trackA = __mul24( ibx + qot, lda ) + res;
    int trackB =  iby+ tx + __mul24(ty , ldb );

    Bb[ty+0*4][tx] = fetch_x_B(trackB+ldb*0, B);
    Bb[ty+1*4][tx] = fetch_x_B(trackB+ldb*4, B);
    Bb[ty+2*4][tx] = fetch_x_B(trackB+ldb*8, B);
    Bb[ty+3*4][tx] = fetch_x_B(trackB+ldb*12, B);
    Abs[qot +16*0][res] = fetch_x_A(trackA +  lda*16*0, A) ;
    Abs[qot +16*1][res] = fetch_x_A(trackA +  lda*16*1, A) ;
    Abs[qot +16*2][res] = fetch_x_A(trackA +  lda*16*2, A) ;
    Abs[qot +16*3][res] = fetch_x_A(trackA +  lda*16*3, A) ;
    __syncthreads();

    const double *Bend = B + k*ldb-16*ldb;
   
    double Axs[4];
    double Bxp[4];

    double Cb[16] = {0,0,0,0,    0,0,0,0, 0,0,0,0, 0,0,0,0};

    do 
    {
    B += 16*ldb;
    A += 16  ;
    trackA+=16 ; 
    trackB+=16*ldb;

    xxB[0]=fetch_x_B( trackB + 0*ldb, B) ;
    xxB[1]=fetch_x_B( trackB + 4*ldb, B) ;
    xxB[2]=fetch_x_B( trackB + 8*ldb, B) ;
    xxB[3]=fetch_x_B( trackB + 12*ldb, B) ;

    xxA[0] =fetch_x_A(trackA +  lda*0*16, A) ;
    xxA[1]=fetch_x_A(trackA +  lda*1*16 , A) ;
    xxA[2]=fetch_x_A(trackA +  lda*2*16 , A) ;
    xxA[3]=fetch_x_A(trackA +  lda*3*16 , A) ;

        #pragma unroll 
        for( int j1=0;j1<16;j1++){
            #pragma unroll
            for( int y=0;y<4;y++)
                Bxp[y]= Bb[j1][qot + y*16];

            #pragma unroll
            for(int y=0;y<4;y++)
                Axs[y] =  Abs[res + y*16][j1];

            #pragma unroll 
            for( int x=0;x<4;x++){
                #pragma unroll 
                for( int y=0;y<4;y++){
                   Cb[x*4+y]  += Axs[x]*Bxp[y];
        }
        }
        }
        
    __syncthreads();
    #pragma unroll
    for(int y=0;y<4;y++)
       Abs[qot+16*y][res] = xxA[y];
 
    #pragma unroll
    for(int y=0;y<4;y++)
       Bb[ty+y*4][tx] =xxB[y];

    __syncthreads();
    } 
    while (B < Bend);

    C += res + ibx  + __mul24 (qot + iby ,ldc);

    #pragma unroll 
    for( int j1=0;j1<16; j1++){
       #pragma unroll
       for( int y=0;y<4;y++)
           Bxp[y]= Bb[j1][qot+y*16];

       #pragma unroll
       for( int y=0;y<4;y++)
           Axs[y] =  Abs[res+y*16][j1];

       #pragma unroll 
       for( int x=0;x<4;x++){
            #pragma unroll 
            for( int y=0;y<4;y++){
               Cb[x*4+y]  += Axs[x]*Bxp[y];
        }
    }
    }

    #pragma unroll
    for( int y=0;y<4;y++){
    #pragma unroll
        for(int x=0; x<4; x++)
       C[x*16] =alpha*Cb[y+x*4] + beta * C[x*16];
           
    C+=ldc*16;
    }
}

__global__ void fermiDgemm_v2_kernel_NT(double *C, const double *A, const double *B,  
                                        int m, int n,  int k,  int lda,  int ldb,  
                                        int ldc, double alpha, double beta) 
{
    const  int tx = threadIdx.x;
    const  int ty = threadIdx.y;

    const int iby = blockIdx.y * 64;
    const int ibx = blockIdx.x * 64;
    const int idt = ty * 64 + tx;

    const int res = idt%16;
    const int qot = idt/16;

    __shared__ double Bb[16][65];
    __shared__ double Abs[64][17];

    double xxA[4];
    double xxB[4];

    B+= iby+ tx + __mul24(ty , ldb );
    int trackB =  iby+ tx + __mul24(ty , ldb );

    A+= ibx +__mul24( qot, lda) + res ; 
    int trackA =  ibx +__mul24( qot, lda) + res ;

    Bb[ty+0*4][tx] = fetch_x_B(trackB+ldb*0, B);
    Bb[ty+1*4][tx] = fetch_x_B(trackB+ldb*4, B);
    Bb[ty+2*4][tx] = fetch_x_B(trackB+ldb*8, B);
    Bb[ty+3*4][tx] = fetch_x_B(trackB+ldb*12, B);

    Abs[res+0*16   ][qot] =fetch_x_A(trackA +  0*16, A) ;
    Abs[res+1*16   ][qot] =fetch_x_A(trackA +  1*16, A) ;
    Abs[res+2*16   ][qot] =fetch_x_A(trackA +  2*16, A) ;
    Abs[res+3*16   ][qot] =fetch_x_A(trackA +  3*16, A) ;
    __syncthreads();

    const double *Bend = B + k*ldb-16*ldb;
   
    double Axs[4];
    double Bxp[4];

    double Cb[16] = {0,0,0,0,    0,0,0,0, 0,0,0,0, 0,0,0,0};

    do 
    {
    B += 16*ldb;
    A += lda *16  ;
    trackA+=16*lda ; 
    trackB+=16*ldb;

    xxB[0]=fetch_x_B( trackB + 0*ldb, B) ;
    xxB[1]=fetch_x_B( trackB + 4*ldb, B) ;
    xxB[2]=fetch_x_B( trackB + 8*ldb, B) ;
    xxB[3]=fetch_x_B( trackB + 12*ldb, B) ;

    xxA[0]=fetch_x_A(trackA +  0*16 , A) ;
    xxA[1]=fetch_x_A(trackA +  1*16 , A) ;
    xxA[2]=fetch_x_A(trackA +  2*16 , A) ;
    xxA[3]=fetch_x_A(trackA +  3*16 , A) ;

        #pragma unroll 
        for( int j1=0;j1<16;j1++){
            #pragma unroll
            for( int y=0;y<4;y++)
                Bxp[y]= Bb[j1][qot+y*16];

            #pragma unroll
            for(int y=0;y<4;y++)
               Axs[y] = Abs[res+y*16][j1] ;

            #pragma unroll 
            for( int x=0;x<4;x++){
                #pragma unroll 
                for( int y=0;y<4;y++){
                   Cb[x*4+y]  += Axs[x]*Bxp[y];
        }
        } 
        }
        
    __syncthreads();
    #pragma unroll
    for(  int y=0;y<4;y++)
        Abs[res+y*16   ][qot] =xxA[y]; 

    #pragma unroll
    for(  int y=0;y<4;y++)
        Bb[ty+y*4][tx] =xxB[y];
    __syncthreads();
    } 
    while (B < Bend);

    C += res + ibx  + __mul24 (qot + iby ,ldc);

    #pragma unroll 
    for( int j1=0;j1<16;j1++){
        #pragma unroll
        for( int y=0;y<4;y++)
            Bxp[y]= Bb[j1][qot+y*16];

        #pragma unroll
        for( int y=0;y<4;y++)
            Axs[y] =  Abs[res+y*16][j1];

        #pragma unroll 
        for( int x=0; x<4; x++){
            #pragma unroll 
            for( int y=0;y<4;y++){
                 Cb[x*4+y]  += Axs[x]*Bxp[y];
        }
    }
    }

    #pragma unroll
    for( int y=0;y<4;y++){
    #pragma unroll
        for(int x=0;x<4;x++)
       C[x*16] =alpha*Cb[y+x*4] + beta * C[x*16];
       
    C+=ldc*16;
    }
}

extern "C" void
magmablas_fermi_dgemm(char TRANSA, char TRANSB, int m , int n , int k , 
                      double alpha, const double *A, int lda, const double *B, 
                      int ldb, double beta, double *C, int ldc ) 
{
   if (m<=0 || n<=0 || k<=0)
     return;

   /*
   if( m % (64) !=0 || n% (64)!=0 || k%(64) !=0 )
   {
    printf("Dimension Should Be multiple of %d\n", 64);
    printf("Calling cublasDgemm\n");
    cublasDgemm(TRANSA, TRANSB, m, n, k, alpha, A, lda, B,ldb,
                    beta, C, ldc);
    return;
   }
   */
   assert(m%64 == 0 && n%64==0 && k%64==0);

   //cudaChannelFormatDesc channelDesc = 
   //    cudaCreateChannelDesc(32, 32, 0, 0, cudaChannelFormatKindSigned);
   //cudaError_t  errt = cudaBindTexture(0,tex_x_double_A,A,channelDesc);
   //if( errt != cudaSuccess) printf("can not bind to texture \n");
   //errt = cudaBindTexture(0,tex_x_double_B,B,channelDesc);
   //if( errt != cudaSuccess) printf("can not bind to texture \n");

   dim3 threads( 64, 4 );
   dim3 grid(m/(64)+(m%(64)!=0),n/(64)+(n%(64)!=0));

  if( TRANSB == 'T' || TRANSB == 't') 
    if( TRANSA == 'N' ||  TRANSA == 'n') 
      fermiDgemm_v2_kernel_NT<<< grid, threads>>>(C, A, B, m, n, k, lda, ldb, 
                                                  ldc, alpha, beta);
    else
      fermiDgemm_v2_kernel_TT<<< grid, threads>>>(C, A, B, m, n, k, lda, ldb, 
                                                  ldc, alpha, beta);
  else
    if( TRANSA == 'N' || TRANSA == 'n') 
      fermiDgemm_v2_kernel_NN<<< grid, threads>>>(C, A, B, m, n, k, lda, ldb, 
                                                  ldc, alpha, beta);
    else
      fermiDgemm_v2_kernel_TN<<< grid, threads>>>(C, A, B, m, n, k, lda, ldb, 
                                                  ldc, alpha, beta);

   //cudaUnbindTexture ( tex_x_double_A ) ;
   //cudaUnbindTexture ( tex_x_double_B ) ;
}

//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================

//all must be column matrices!
static void do_magma_dgemm(double *C, double *A, double *B,
  IDX N, IDX M, IDX P, bool A_trans, bool B_trans)
{
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

  magmablas_fermi_dgemm((A_trans ? 'T' : 'N'), (B_trans ? 'T' : 'N'), N, M, P, 
    alpha, dA, N, dB, P, beta, dC, N);

  //transfer C to host
  cudaMemcpy(C, dC, C_size, cudaMemcpyDeviceToHost);  

  //free matrices on device
  cudaFree(dC);
  cudaFree(dA);
  cudaFree(dB);
}

static void do_test(IDX N, IDX M, IDX P, bool A_trans, bool B_trans)
{
  double *C, *A, *B;
  C = create_matrix_dc(N, M, FILL);
  A = create_matrix_dc(N, P, FILL);
  B = create_matrix_dc(P, M, FILL);

  double flop = GEMM_FLOP(N, M, P);

  double magma_start = get_real_time();
  do_magma_dgemm(C, A, B, N, M, P, A_trans, B_trans);
  double magma_duration = get_real_time() - magma_start;

  PRINT("N=%4d M=%4d P=%4d (%s:%s) | MAGMA took % 10.6f secs | ", 
    N, M, P,
    (A_trans ? "Atrns" : "Anorm"), (B_trans ? "Btrns" : "Bnorm"), magma_duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ magma_duration / 1e6);

  double naive_start = get_real_time();
  do_cuda_dgemm(C, A, B, N, M, P, A_trans, B_trans);
  double naive_duration = get_real_time() - naive_start;

  PRINT("N=%4d M=%4d P=%4d (%s:%s) | NAIVE took % 10.6f secs | ", 
    N, M, P,
    (A_trans ? "Atrns" : "Anorm"), (B_trans ? "Btrns" : "Bnorm"), naive_duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ naive_duration / 1e6);

  printf("results match\n");

  free_matrix_d(C);
  free_matrix_d(A);
  free_matrix_d(B);
}

int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX nmp_start = NMP_MIN;
  IDX nmp_end = NMP_MAX;
  IDX idx = -1;

  unsigned tmp_seed;
  IDX tmp_nmp;
  IDX tmp_idx;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(sscanf(argv[i], "--size=%u", &tmp_nmp) == 1){
      nmp_start = tmp_nmp;
      nmp_end = tmp_nmp;
    }
    else if(sscanf(argv[i], "--idx=%d", &tmp_idx) == 1){
      if(tmp_idx > 3 || tmp_idx < 0) {
        printf("invalid idx, must be [0,3]\n");
        exit(0);
      }
      idx = tmp_idx;
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
    if(idx == -1) {
      do_test(NMP, NMP, NMP, NO_TRANS,  NO_TRANS);
      do_test(NMP, NMP, NMP, NO_TRANS,  YES_TRANS);
      do_test(NMP, NMP, NMP, YES_TRANS, NO_TRANS);
      do_test(NMP, NMP, NMP, YES_TRANS, YES_TRANS);
    } else if(idx == 0) {
      do_test(NMP, NMP, NMP, NO_TRANS,  NO_TRANS);
    } else if(idx == 1) {
      do_test(NMP, NMP, NMP, NO_TRANS,  YES_TRANS);
    } else if(idx == 2) {
      do_test(NMP, NMP, NMP, YES_TRANS, NO_TRANS);
    } else {
      do_test(NMP, NMP, NMP, YES_TRANS, YES_TRANS);
    }

    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
