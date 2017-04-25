//=========================================================================
//naive dtrsm c file
//
//Results:
// gsl runs at about ~1.2 GFLOP/s on my machine regardless of input sizes
// my single threaded results are ~3.6 GFLOP/s at > 64x64 matrices
// on average, single threaded 2-3x speedup
// 
//=========================================================================

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <intrin.h>

#include "trsm.h"
#include "get_real_time.h"
#include "gsl_result_check.h"
#include "matrix_utilities.h"

#define NMP_MIN 8
#define NMP_MAX 2048
#define NMP_SCALE 2


//==========================================================================
// Thread routines
//==========================================================================

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
      if (Noffset >= N) {
        break;
      }
      targs[ts].C = C;
      targs[ts].A = A_trans;
      targs[ts].B = B;
      targs[ts].N = N;
      targs[ts].M = M;
      targs[ts].P = P;
      targs[ts].Noffset = Noffset;
      targs[ts].Ncount = min(Ncount, N - Noffset);
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

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

//only do lower for now
void do_test(IDX M, IDX P, bool right, bool upper, bool unit, bool trans)
{
  double *A, *B, *B_orig, *gsl_A, *gsl_B, *gsl_B_solved, *gsl_B_orig;
  IDX B_rows = right ? P : M;
  IDX B_cols = right ? M : P;
  bool real_upper = trans ? !upper : upper;
  double alpha = get_scalar_double();

  //algorithm requirement #2 ('right' determines row order and indices)
  if (right) {
    A = create_matrix_trianglular_dc(M, M, upper, unit);
    B = create_matrix_dr(P, M, FILL);
    B_orig = copy_matrix_dr(B, P, M);
  } 
  else {
    A = create_matrix_trianglular_dr(M, M, upper, unit);
    B = create_matrix_dc(M, P, FILL);
    B_orig = copy_matrix_dc(B, M, P);
  }

  double start = getRealTime();
  double *A_actual = trans ? transpose_matrix_dcr(A, M, M) : A;
  do_trsm(A_actual, B, M, B_rows, B_cols, right, real_upper, unit, alpha);
  if (trans) {
    destroy_matrix_d(A_actual);
  }
  double duration = getRealTime() - start;

  double flop = unit ? TRSM_FLOP_UNIT(M, P) : TRSM_FLOP_DIAG(M, P);
  printf("M=%4u P=%4u %s%s%s%s | took % 10.6f secs | ", M, P, 
    (right ? "R" : "L"), (upper ? "U" : "L"), 
    (trans ? "T" : "N"), (unit ? "U" : "D"), duration);
  printf("% 10.3f MFLOP | ", flop/1e6);
  printf("% 10.3f MFLOP/s || ", flop/(duration*1e6));

  if (right) {
    gsl_A = transpose_matrix_dcr(A, M, M);
    gsl_B = copy_matrix_dr(B_orig, P, M);
    gsl_B_orig = copy_matrix_dr(B_orig, P, M);
    gsl_B_solved = copy_matrix_dr(B, P, M);
  }
  else {
    gsl_A = copy_matrix_dr(A, M, M);
    gsl_B = transpose_matrix_dcr(B_orig, M, P);
    gsl_B_orig = transpose_matrix_dcr(B_orig, M, P);
    gsl_B_solved = transpose_matrix_dcr(B, M, P);
  }
  verify_gsl_dtrsm(gsl_A, gsl_B, gsl_B_orig, gsl_B_solved,
    M, P, right, upper, unit, trans, alpha);

  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(B_orig);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
  destroy_matrix_d(gsl_B_orig);
}


//==========================================================================
// TEST ENTRYPOINT -- TEST SWEEPS MULTIPLE PARAMETERS (all 16 versions)
//==========================================================================

int main(int argc, char **argv)
{
  //set breakpoint here, then make command window huge
  srand((unsigned)time(NULL));

  for (IDX NMP = NMP_MIN; NMP <= NMP_MAX; NMP *= NMP_SCALE) {
    for (bool trans = A_TRANS; trans == true; trans = A_NOT_TRANS) {
      do_test(NMP, NMP, A_RIGHT, A_UPPER, A_UNIT, trans);
      do_test(NMP, NMP, A_RIGHT, A_UPPER, A_NOT_UNIT, trans);
      do_test(NMP, NMP, A_RIGHT, A_LOWER, A_UNIT, trans);
      do_test(NMP, NMP, A_RIGHT, A_LOWER, A_NOT_UNIT, trans);
      do_test(NMP, NMP, A_LEFT, A_UPPER, A_UNIT, trans);
      do_test(NMP, NMP, A_LEFT, A_UPPER, A_NOT_UNIT, trans);
      do_test(NMP, NMP, A_LEFT, A_LOWER, A_UNIT, trans);
      do_test(NMP, NMP, A_LEFT, A_LOWER, A_NOT_UNIT, trans);
    }
  }
  printf("\n");

  //set breakpoint here, then analyze results
  return 0;
}