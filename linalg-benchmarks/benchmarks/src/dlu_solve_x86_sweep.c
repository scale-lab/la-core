//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <Eigen/Dense>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

static const IDX NP_MIN = 4;
static const IDX NP_MAX = 1024;
static const IDX NP_SCALE = 2;

//==========================================================================

static void do_test_eigen(IDX N, IDX P)
{
  using namespace Eigen;
  using namespace std;

  double start = get_real_time();
  MatrixXd A = MatrixXd::Random(N, N);
  MatrixXd B = MatrixXd::Random(N, P);
  MatrixXd X = A.lu().solve(B);
  double duration = get_real_time() - start;

  double flop = LU_SOLVE_FLOP(N);
  PRINT("N=%4d P=%4d | EIGEN took % 10.6f secs | ", N, P, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration / 1e6);
}

static void do_test(IDX N, IDX P)
{
  double *A, *B;
  A = create_matrix_dr(N, N, FILL);
  B = create_matrix_dc(N, P, FILL);

  double start = get_real_time();
  gsl_matrix_view gsl_A = gsl_matrix_view_array(A, N, N);
  int s;
  gsl_permutation * p = gsl_permutation_alloc(N);
  gsl_linalg_LU_decomp (&gsl_A.matrix, p, &s);

  for(IDX i=0; i<P; ++i) {
    gsl_vector_view gsl_B = gsl_vector_view_array (B+(i*N), N);
    gsl_vector *x = gsl_vector_alloc(N);
    gsl_linalg_LU_solve (&gsl_A.matrix, p, &gsl_B.vector, x);
    memcpy(B+(i*N), x->data, sizeof(double)*N);
    gsl_vector_free(x);
  }
  gsl_permutation_free(p);
  double duration = get_real_time() - start;

  double flop = LU_SOLVE_FLOP(N) / 1e6;
  PRINT("N=%4d P=%4d | GSL took % 10.6f secs | ", N, P, duration);
  PRINT("% 9.3f MFLOP | ", flop);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration);

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
    do_test(NP, 1);//NP);
    do_test_eigen(NP, 1);//NP);
  }

  //set breakpoint here, then analyze results
  return 0;
}
