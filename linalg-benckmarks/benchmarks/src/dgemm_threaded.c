//dgemm c file

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>

#include "get_real_time.h"
#include "gsl_result_check.h"

#define BLOCK_MIN 8
#define BLOCK_MAX 64
#define BLOCK_SCALE 2

#define N_MIN 32
#define N_MAX 2048
#define N_SCALE 8

#define M_MIN 32
#define M_MAX 2048
#define M_SCALE 8

#define P_MIN 32
#define P_MAX 2048
#define P_SCALE 8

#define THREAD_MIN 1
#define THREAD_MAX 16
#define THREAD_SCALE 2

//NOTE: we are starting at block size of 8, not 1. Block size of 1 is
//atrociously slow
void dgemm_block(double *A, double *B, double *C, int M, int N, int P, int BS,
  int i, int j, int k)
{
  int min_i = i;
  int min_j = j;
  int min_k = k;
  int max_i = (i + BS > N) ? N : (i + BS);  //N!
  int max_j = (j + BS > M) ? M : (j + BS);  //M!
  int max_k = (k + BS > P) ? P : (k + BS);

  for(i=min_i; i<max_i; ++i){
    for(j=min_j; j<max_j; ++j){
      for(k=min_k; k<max_k; ++k){
        if (k == 0) {
          C[i*M + j] = 0;
        }
        C[i*M+j] += A[i*P+k]*B[k*M+j];
      }
    }
  }
}

// C[n][m] = A[n][p] * B[p][m]
void dgemm(double *A, double *B, double *C, int M, int N, int P, int BS)
{
  for (int i=0; i<N; i += BS) {
    for (int j=0; j<M; j += BS) {
      for (int k=0; k<P; k += BS) {
        dgemm_block(A, B, C, M, N, P, BS, i, j, k);
      }
    }
  }
}


double * create_matrix(int h, int w, bool fill)
{
  double *m = (double *)malloc(sizeof(double)*h*w);

  if (fill) {
    for (int i = 0; i<h; ++i) {
      for (int j = 0; j<w; ++j) {
        m[i*w + j] = ((double)rand() / (double)RAND_MAX);
      }
    }
  }
  return m;
}

void destroy_matrix(double *m)
{
  free(m);
}

typedef struct ThreadData {
  double *A;
  double *B;
  double *C;
  int M;
  int N;
  int P;
  int BS;
  unsigned long thread_id;
  void * thread;
} ThreadData;


DWORD WINAPI thread_func(void *args)
{
  ThreadData * targs = (ThreadData *)args;
  dgemm(targs->A, targs->B, targs->C, targs->M, targs->N, targs->P, targs->BS);
  return 0;
}

// including the malloc() time in performance counting!
void run_threads(double *A, double *B, double *C, int M, int N, int P, 
  int BS, int THREADS)
{
  ThreadData * targs;

  if (THREADS == 1) {
    dgemm(A, B, C, M, N, P, BS);
  }
  else {
    targs = (ThreadData *)malloc(THREADS * sizeof(ThreadData));

    for (int i = 0; i < THREADS; ++i) {
      int chunk = (N / THREADS);
      int offset = i*chunk;
      int new_N = ((offset + chunk) > N) ? (N - offset) : chunk;
      targs[i].A = A + (offset * P);
      targs[i].B = B;
      targs[i].C = C + (offset * M);
      targs[i].M = M;
      targs[i].N = chunk;
      targs[i].P = P;
      targs[i].BS = BS;
      targs[i].thread_id = 0;
      targs[i].thread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        thread_func,            // thread function name
        &(targs[i]),            // argument to thread function 
        0,                      // use default creation flags 
        &(targs[i].thread_id)
      );
      if (targs[i].thread == NULL) {
        printf("CreateThread failed\n");
        ExitProcess(3);
      }
    }
    for (int i = 0; i < THREADS; ++i) {
      WaitForSingleObject(targs[i].thread, INFINITE);
    }
  }
}

void do_test(int N, int M, int P, int BS, int THREADS)
{
  double * A = create_matrix(N, P, true);
  double * B = create_matrix(P, M, true);
  double * C = create_matrix(N, M, false);


  double start = getRealTime();
  run_threads(A, B, C, M, N, P, BS, THREADS);
  double duration = getRealTime() - start;

  printf("N=%d M=%d P=%d BS=%d THREADS=%d, took %.6f seconds | ", 
    N, M, P, BS, THREADS, duration);
  printf("%d KFLOP | ", ((2*P - 1)*N*M) / 1000);
  printf("%.3f MFLOP/s | ", ((double)((2*P - 1)*N*M)) / duration / 1e6);

  verify_gsl_dgemm(A, B, C, M, N, P);
  destroy_matrix(A);
  destroy_matrix(B);
  destroy_matrix(C);
}


int main(int argc, char **argv)
{
  srand((unsigned)time(NULL));

  for(int N=N_MIN; N<=N_MAX; N *= N_SCALE){
    for(int M=M_MIN; M<=M_MAX; M*= M_SCALE){
      for(int P=P_MIN; P<=P_MAX; P *= P_SCALE){
        for(int BS=BLOCK_MIN; BS<=BLOCK_MAX; BS *= BLOCK_SCALE){
          for (int T = THREAD_MIN; T <= THREAD_MAX; T *= THREAD_SCALE) {
            do_test(N, M, P, BS, T);
          }
        }
        for (int BS=BLOCK_MAX/BLOCK_SCALE; BS>=BLOCK_MIN; BS /= BLOCK_SCALE) {
          for (int T = THREAD_MIN; T <= THREAD_MAX; T *= THREAD_SCALE) {
            do_test(N, M, P, BS, T);
          }
        }
        printf("\n");
      }
    }
  }

  return 0;
}