//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <string>
#include <vector>

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include <gsl/gsl_spmatrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_spblas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;

//==========================================================================

static void do_eigen_test(IDX size, double sparsity)
{
  using namespace Eigen;
  using namespace std;

  double *A_data_ptr;
  uint32_t *A_col_ptr;
  uint32_t *A_row_ptr;
  MatrixXd b = MatrixXd::Random(size, 1);
  MatrixXd x;

  create_matrix_spv_dr(size, size, sparsity,
                       &A_data_ptr, &A_col_ptr, &A_row_ptr);

  std::vector< Eigen::Triplet<double> > tripletList;
  IDX i, j, offset_j;
  for (i = 0; i < size; ++i) {
    IDX row_items = (i == 0 ? A_row_ptr[i] : A_row_ptr[i]-A_row_ptr[i-1]);
    IDX row_offset = (i == 0 ? 0 : A_row_ptr[i-1]);
    for (j = 0; j < size; ++j) {
      for (offset_j = row_offset; offset_j<(row_offset+row_items); ++offset_j) {
        if(A_col_ptr[offset_j] == j) {
          tripletList.push_back(Eigen::Triplet<double>(i, j, A_data_ptr[offset_j]));
          break;
        }
      }
    }
  }
  SparseMatrix<double,RowMajor> mat(size,size);
  mat.setFromTriplets(tripletList.begin(), tripletList.end());

  if(DEBUG){
    dump_matrix_spv_dr("A", size, size, A_data_ptr, A_col_ptr, A_row_ptr);
    std::cout << "mat=[\n" << mat << "]\n" << std::endl;
    std::cout << "b=[\n" << b << "]\n" << std::endl;
  }

  double start = get_real_time();
  x = mat*b;
  double duration = get_real_time() - start;

  if(DEBUG){
    std::cout << "X=[\n" << x << "]\n" << std::endl;
  }

  double flop = GEMM_FLOP(size, size, 1);
  PRINT("size=%4d | Eigen took % 10.6f secs | ", size, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop / duration / 1e6);

  free(A_data_ptr);
  free(A_row_ptr);
  free(A_col_ptr);
}

static void do_gsl_test(IDX size, double sparsity)
{
  double *A_data_ptr;
  uint32_t *A_col_ptr;
  uint32_t *A_row_ptr;

  double *b, *x;

  create_matrix_spv_dr(size, size, sparsity,
                       &A_data_ptr, &A_col_ptr, &A_row_ptr);
  b = create_matrix_dc(size, 1, true);
  x = create_matrix_dc(size, 1, false);

  if(DEBUG){
    dump_matrix_spv_dr("A", size, size, A_data_ptr, A_col_ptr, A_row_ptr);
    dump_matrix_dc("b", b, size, 1);
  }

  //==========================================================================
  gsl_spmatrix *gsl_A = gsl_spmatrix_alloc(size, size);
  IDX i, j, offset_j;
  for (i = 0; i < size; ++i) {
    IDX row_items = (i == 0 ? A_row_ptr[i] : A_row_ptr[i]-A_row_ptr[i-1]);
    IDX row_offset = (i == 0 ? 0 : A_row_ptr[i-1]);
    for (j = 0; j < size; ++j) {
      for (offset_j = row_offset; offset_j<(row_offset+row_items); ++offset_j) {
        if(A_col_ptr[offset_j] == j) {
          gsl_spmatrix_set(gsl_A, i, j, A_data_ptr[offset_j]);
          break;
        }
      }
    }
  }
  //==========================================================================
  double start = get_real_time();
  gsl_vector_view gsl_b = gsl_vector_view_array(b, size);
  gsl_vector_view gsl_x = gsl_vector_view_array(x, size);
  gsl_spblas_dgemv(CblasNoTrans, 1.0, gsl_A, 
                   &gsl_b.vector, 0.0, &gsl_x.vector);
  double duration = get_real_time() - start;
  //==========================================================================

  if(DEBUG){
    dump_matrix_dc("x", x, size, 1);
  }

  double flop = GEMM_FLOP(size, size, 1);
  PRINT("size=%4d | GSL took % 10.6f secs | ", size, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s\n", flop / duration / 1e6);

  free(A_data_ptr);
  free(A_row_ptr);
  free(A_col_ptr);
  free(b);
  free(x);
}


int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX size = 0;
  double sparsity = -1.0;


  unsigned tmp_seed;
  double tmp_sparsity;
  IDX tmp_size;
  uint64_t tmp_scratch_size;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(!strcmp(argv[i], "--debug")){
      DEBUG = true;
    }
    else if(sscanf(argv[i], "--size=%u", &tmp_size) == 1){
      size = tmp_size;
    }
    else if(sscanf(argv[i], "--sparsity=%lf", &tmp_sparsity) == 1){
      sparsity = tmp_sparsity;
      printf("[dgemv_spv]: sparsity is %lf\n", sparsity);
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //---------------------------------------------------------------------------
  if (size == 0) {
    printf("matrix size must be greater than 0\n");
    exit(0);
  }
  if (sparsity < 0.0 || sparsity > 1.0) {
    printf("sparsity must be between 0.0-1.0\n");
    exit(0);
  }

  //---------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //---------------------------------------------------------------------------

  //set breakpoint here, then make command window huge
  do_eigen_test(size, sparsity);
  do_gsl_test(size, sparsity);

  //set breakpoint here, then analyze results
  return 0;
}
