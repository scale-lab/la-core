//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <Eigen/Dense>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

//==========================================================================

static bool DEBUG = false;

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;


//==========================================================================

static void do_test_eigen(IDX N)
{
  using namespace Eigen;
  using namespace std;

  MatrixXd A = MatrixXd::Random(N, N);
  MatrixXd B = MatrixXd::Random(N, N);

  if(DEBUG){
    std::cout << "EIGEN: A before:\n" << A << "\n";
    std::cout << "EIGEN: B before:\n" << B << "\n";
  }
  double start = get_real_time();
  A = (A.transpose()+B).eval();
  double duration = get_real_time() - start;

  if(DEBUG){
    std::cout << "EIGEN: A after:\n" << A << "\n";
  }
  double flop = PTRANS_FLOP(N);
  PRINT("N=%4d | Eigen took % 10.6f secs | ",  N, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop/ duration / 1e6);

}

//A and B row major
static void do_test_custom(double *A, double *B, IDX N)
{ 
  IDX BLOCK = 4;

  for(IDX i=0; i<N; i+=BLOCK) {
    for(IDX j=i; j<N; j+=BLOCK) {

      for(IDX ik=0; ik<BLOCK; ++ik) {
        for(IDX jk=0; jk<BLOCK; ++jk) {
          double tmp = IDXR(A, j+jk, i+ik, N, N) + IDXR(B, i+ik, j+jk, N, N);
          IDXR(A, j+jk, i+ik, N, N) = 
            IDXR(A, i+ik, j+jk, N, N) + IDXR(B, j+jk, i+ik, N, N);
          IDXR(A, i+ik, j+jk, N, N) = tmp;
        }
      }
    }
  }
}


static void do_test(IDX N)
{
  double *A, *A_custom, *B;
  B = create_matrix_dr(N, N, FILL);
  A = create_matrix_dr(N, N, FILL);
  A_custom = copy_matrix_dr(A, N, N);

  if(DEBUG){
    dump_matrix_dr("CUSTOM: A before", A_custom, N, N);
    dump_matrix_dr("GSL: A before", A, N, N);
    dump_matrix_dr("GSL: B before", B, N, N);
  }

//==========================================================================
  double start = get_real_time();
  do_test_custom(A_custom, B, N);
  double duration = (get_real_time() - start)*1e6;

  double flop = PTRANS_FLOP(N);
  PRINT("N=%4d | CUSTOM took % 10.6f secs | ",  N, duration);
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

  if(DEBUG){
    dump_matrix_dr("CUSTOM: A after", A_custom, N, N);
    dump_matrix_dr("GSL: A after", A, N, N);
  }

  free_matrix_d(A_custom);
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
    do_test_eigen(NMP);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
