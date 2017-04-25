//dgemm c file

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BLOCK_SIZE_MIN = 1;
#define BLOCK_SIZE_MAX = 16;
#define BLOCK_SIZE_SCALE = 2;

#define N_MIN 32
#define N_MAX 2048
#define N_SCALE 8

#define M_MIN 32
#define M_MAX 2048
#define M_SCALE 8

#define P_MIN 32
#define P_MAX 2048
#define P_SCALE 8


void dgemm_block(double *A, double *B, double *C, int M, int N, int P, int BS,
  int i, int j, int k)
{
  int max_i = (i + BS > M) ? M : (i + BS);
  int max_j = (j + BS > N) ? N : (j + BS);
  int max_k = (k + BS > P) ? P : (k + BS);

  for(; i<max_i; ++i){
    for(; j<max_i; ++i){
      C[i*n+j] = 0;
      for(; k<max_i; ++i){
        C[i*N+j] += A[i*N+k]*B[k*M+j];
      }
    }
  }
}

// C[n][m] = A[n][p] * B[p][m]
void dgemm(double *A, double *B, double *C, int M, int N, int P, int BS)
{
  for(int i=0; i<M; i += BS){
    for(int j=0; j<N; j += BS){
      for(int k=0; k<P; k += BS){
        dgemm_block(A, B, C, M, N, P, BS, i, j, k);
      }
    }
  }
}


void do_test(double *A, double *B, double *C, int N, int M, int P, int BS)
{
  struct timespec tstart={0,0};
  struct timespec tend={0,0};
  clock_gettime(CLOCK_MONOTONIC, &tstart);

  dgemm(A, B, C, N, M, P, BS);

  clock_gettime(CLOCK_MONOTONIC, &tend);
  double *duration = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
                     ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec)
  printf("N=%d M=%d P=%d BS=%d, took %.6f seconds\n", N, M, P, BS, duration);
}

double * create_matrix(int h, int w, bool fill)
{
  double *m = (double *)malloc(sizeof(double)*N*P); 

  if(fill){
    for(int i=0; i<h; ++i){
      for(int j=0; j<w; ++j){
        a[i*h+w] = ((double)rand()/(double)RAND_MAX);
      }
    }
  }
  return m;
}

void destroy_matrix(double *m)
{
  free(m);
}


int main(int argc, char **argv)
{
  srand(time(NULL));

  double *A;
  double *B;
  double *C;

  for(int N=N_MIN; N<=N_MAX; N *= N_SCALE){
    for(int M=M_MIN; M<=M_MAX; M*= M_SCALE){
      for(int P=P_MIN; P<=P_MAX; P *= P_SCALE){

        for(int BS=BLOCK_SIZE_MIN; BS<BLOCK_SIZE_MAX; BS *= BLOCK_SIZE_SCALE){
          A = create_matrix(N, P, true);
          B = create_matrix(P, M, true);
          C = create_matrix(N, M, false);

          do_test(A, B, C, N, M, P, BS);

          destroy_matrix(A);
          destroy_matrix(B);
          destroy_matrix(C);     
        }
      }
    }
  }

  return 0;
}