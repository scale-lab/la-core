//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "gemv_spv/dgemv_spv_la_core.h"
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/dgemv_spv.h"

//==========================================================================
// Definitions of global control flags
bool DEBUG = false;
uint64_t SCRATCH_SIZE = 0;

//==========================================================================

static void do_test(IDX size, double sparsity)
{
  double *A_data_ptr;
  uint32_t *A_col_ptr;
  uint32_t *A_row_ptr;

  double *b, *x, *x_gsl;

  create_matrix_spv_dr(size, size, sparsity,
                       &A_data_ptr, &A_col_ptr, &A_row_ptr);
  b     = create_matrix_dc(size, 1, true);
  x     = create_matrix_dc(size, 1, false);
  x_gsl = create_matrix_dc(size, 1, false);

  if(DEBUG){
    dump_matrix_spv_dr("A", size, size, A_data_ptr, A_col_ptr, A_row_ptr);
    dump_matrix_dc("b", b, size, 1);
  }

  double start = get_real_time();
  dgemv_spv_la_core(A_data_ptr, A_col_ptr, A_row_ptr, b, x, size, size);
  double duration = get_real_time() - start;

  if(DEBUG){
    dump_matrix_dc("x", x, size, 1);
  }

  double flop = GEMM_FLOP(size, size, 1);
  PRINT("size=%4d | took % 10.6f secs | ", size, duration);
  PRINT("% 9.3f MFLOP | ", flop / 1e6);
  PRINT("% 12.3f MFLOP/s || ", flop / duration / 1e6);

  verify_dgemv_spv(A_data_ptr, A_col_ptr, A_row_ptr, b, x, x_gsl, size, size);

  free(A_data_ptr);
  free(A_row_ptr);
  free(A_col_ptr);
  free(b);
  free(x);
  free(x_gsl);
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
    else if(sscanf(argv[i], "--scratch_size=%llu", &tmp_scratch_size) == 1){
      if (tmp_scratch_size == 0) {
        printf("invalid log2 scratch_size");
        exit(0);
      }
      SCRATCH_SIZE = (1 << tmp_scratch_size);
      printf("[dgemv_spv]: scratch_size is %llu\n", SCRATCH_SIZE);
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
  do_test(size, sparsity);

  //set breakpoint here, then analyze results
  return 0;
}
