//=========================================================================
//naive dgemm c file
//
//Results:
// gsl runs at about ~1.2 GFLOP/s on my machine regardless of input sizes
// my single threaded results are ~800 MFLOP/s at 16x16x16 and ~1.2 GFLOP/s
//   at 1024x1024x1024.
// my multithreaded best is 3.3 GFLOP/s
// my multithreaded with AVX intrinsics is 4.9 GFLOP/s
// 
//=========================================================================

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "gepp.h"
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

#define THREAD_MIN 1
#define THREAD_MAX 16
#define THREAD_SCALE 2

typedef struct ThreadData {
  double *C;
  double *A;
  double *B;
  IDX N;
  IDX M;
  IDX P;
  IDX Noffset;
  IDX Ncount;
  IDX Moffset;
  IDX Mcount;
  unsigned long thread_id;
  void * thread;
} ThreadData;


DWORD WINAPI thread_func(void *args)
{
  ThreadData * targs = (ThreadData *)args;
  gepp_d(targs->C, targs->A, targs->B, targs->N, targs->M, targs->P,
    targs->Noffset, targs->Ncount, targs->Moffset, targs->Mcount
  );
  return 0;
}

//emperically, when going multithreaded makes the most sense
#define THREADED_THRESHOLD 10e6

// including the malloc() time in performance counting!
void run_threads(double *C, double *A, double *B, IDX N, IDX M, IDX P, 
  IDX THREADS)
{
  double *A_trans = transpose_matrix_dcr(A, N, P);
  IDX Ncount = min(max(N / THREADS, BLOCK_D), N);

  ThreadData * targs;
  if (THREADS == 1 || GEMM_FLOP(N, M, P) < THREADED_THRESHOLD) {
    gepp_d(C, A_trans, B, N, M, P, 0, N, 0, M, 0, P);
    destroy_matrix_d(A_trans);
  }
  else {
    targs = (ThreadData *)malloc(THREADS * sizeof(ThreadData));
    IDX ts;
    for (ts = 0; ts < THREADS; ++ts) {
      IDX Noffset = ts*Ncount;
      if (Noffset >= N){
        break;
      }
      targs[ts].C = C;
      targs[ts].A = A_trans;
      targs[ts].B = B;
      targs[ts].N = N;
      targs[ts].M = M;
      targs[ts].P = P;
      targs[ts].Noffset = Noffset;
      targs[ts].Ncount = min(Ncount, N-Noffset);
      targs[ts].Moffset = 0;
      targs[ts].Mcount = M;
      targs[ts].thread_id = 0;        //set below
      targs[ts].thread = CreateThread(
        NULL,                         // default security attributes
        0,                            // use default stack size  
        thread_func,                  // thread function name
        &(targs[ts]),                 // argument to thread function 
        0,                            // use default creation flags 
        &(targs[ts].thread_id)
      );
      if (targs[ts].thread == NULL) {
        printf("CreateThread failed\n");
        ExitProcess(3);
      }
    }
    for (IDX i = 0; i < ts; ++i) {
      WaitForSingleObject(targs[i].thread, INFINITE);
    }
    destroy_matrix_d(A_trans);
  }
}

void do_test(IDX N, IDX M, IDX P, IDX THREADS)
{
  double *C, *A, *B, *gsl_C, *gsl_A, *gsl_B;

  C = create_matrix_dc(N, M, NO_FILL);
  A = create_matrix_dc(N, P, FILL);
  B = create_matrix_dc(P, M, FILL);

  double start = getRealTime();
  run_threads(C, A, B, N, M, P, THREADS);
  double duration = getRealTime() - start;

  double flop = GEMM_FLOP(N, M, P);
  printf("N=%4u M=%4u P=%4u T=%3u | took % 10.6f secs | ", N, M, P,
    THREADS, duration);
  printf("% 10.3f MFLOP | ", flop / 1e6);
  printf("% 10.3f MFLOP/s || ", flop / (duration*1e6));

  gsl_C = transpose_matrix_dcr(C, N, M);
  gsl_A = transpose_matrix_dcr(A, N, P);
  gsl_B = transpose_matrix_dcr(B, P, M);
  verify_gsl_dgemm(gsl_C, gsl_A, gsl_B, N, M, P);

  destroy_matrix_d(C);
  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(gsl_B);
  destroy_matrix_d(gsl_C);
  destroy_matrix_d(gsl_A);
}


int main(int argc, char **argv)
{
  //set breakpoint here, then make command window huge
  srand((unsigned)time(NULL));

  for (IDX NMP = NMP_MIN; NMP <= NMP_MAX; NMP *= NMP_SCALE) {
    for (IDX T = THREAD_MIN; T <= THREAD_MAX; T *= THREAD_SCALE) {
      do_test(NMP, NMP, NMP, T);
    }
  }
  printf("\n");

  //set breakpoint here, then analyze results
  return 0;
}