//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <gsl/gsl_matrix.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
#include "verify/common.h"

//==========================================================================

static bool DEBUG = false;
uint64_t SCRATCH_SIZE = 0;

static const IDX NMP_MIN        = 128; //was 2
static const IDX NMP_MAX        = 1024;
static const IDX NMP_SCALE      = 2;


//==========================================================================

static void do_test_lacore(double *A, double *B, IDX N)
{
  //IDX BLOCK = (IDX)sqrt((double)(SCRATCH_SIZE/2/sizeof(double)));
  IDX BLOCK = 32;
  IDX BLOCK_SQAR = BLOCK*BLOCK;
  IDX BLOCK_N = BLOCK*N;
  IDX N_SUB_BLOCK = N-BLOCK;
  IDX ONE_SUB_BLOCK_N = 1-BLOCK_N;

  LaAddr AIJ_SCH_ADR = 0;
  LaAddr AJI_SCH_ADR = SCRATCH_SIZE/2;

  LaRegIdx One_REG     = 0;
  LaRegIdx Aij_mem_REG = 1;
  LaRegIdx Aji_mem_REG = 2;
  LaRegIdx Aij_sch_REG = 3;
  LaRegIdx Aji_sch_REG = 4;
  LaRegIdx Bij_mem_REG = 5;
  LaRegIdx Bji_mem_REG = 6;

  la_set_scalar_dp_reg(One_REG, 1.0);
  for(IDX i=0; i<N; i+=BLOCK) {
    for(IDX j=i; j<N; j+=BLOCK) {
      //get sub-block indices
      LaAddr Aij = (LaAddr)&IDXR(A, i, j, N, N);
      LaAddr Aji = (LaAddr)&IDXR(A, j, i, N, N);
      LaAddr Bij = (LaAddr)&IDXR(B, i, j, N, N);
      LaAddr Bji = (LaAddr)&IDXR(B, j, i, N, N);

      //set the mem and scratch locations
      la_set_vec_dp_mem(Aij_mem_REG, Aij, N, BLOCK, ONE_SUB_BLOCK_N);
      la_set_vec_dp_mem(Aji_mem_REG, Aji, N, BLOCK, ONE_SUB_BLOCK_N);
      la_set_vec_dp_sch(Aij_sch_REG, AIJ_SCH_ADR, 1, BLOCK, 0);
      la_set_vec_dp_sch(Aji_sch_REG, AJI_SCH_ADR, 1, BLOCK, 0);
      la_set_vec_dp_mem(Bij_mem_REG, Bij, 1, BLOCK, N_SUB_BLOCK);
      la_set_vec_dp_mem(Bji_mem_REG, Bji, 1, BLOCK, N_SUB_BLOCK);

      //transpose sub-blocks into scratch, then execute
      la_copy(Aij_sch_REG, Aij_mem_REG, BLOCK_SQAR);
      la_copy(Aji_sch_REG, Aji_mem_REG, BLOCK_SQAR);

      la_set_vec_dp_mem(Aij_mem_REG, Aij, 1, BLOCK, N_SUB_BLOCK);
      la_set_vec_dp_mem(Aji_mem_REG, Aji, 1, BLOCK, N_SUB_BLOCK);
      la_AaddBmulC(Aij_mem_REG, Aji_sch_REG, Bij_mem_REG, One_REG, BLOCK_SQAR);
      la_AaddBmulC(Aji_mem_REG, Aij_sch_REG, Bji_mem_REG, One_REG, BLOCK_SQAR);
    }
  }
}


static void do_test(IDX N)
{
  double *A, *A_lacore, *B;
  B = create_matrix_dr(N, N, FILL);
  A = create_matrix_dr(N, N, FILL);
  A_lacore = copy_matrix_dr(A, N, N);

  if(DEBUG){
    dump_matrix_dr("LACORE: A before", A_lacore, N, N);
    dump_matrix_dr("GSL: A before", A, N, N);
    dump_matrix_dr("GSL: B before", B, N, N);
  }

//==========================================================================
  double start = get_real_time();
  do_test_lacore(A_lacore, B, N);
  double duration = (get_real_time() - start)*1e6;

  double flop = PTRANS_FLOP(N);
  PRINT("N=%4d | LACORE took % 10.6f secs | ",  N, duration);
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

  for(IDX i=0; i<N; ++i){
    for(IDX j=0; j<N; ++j){
      double num1 = IDXR(A, i, j, N, N);
      double num2 = IDXR(A_lacore, i, j, N, N);
      if(MARGIN_EXCEEDED(num1, num2)){
        printf("Matrices differ at [%d,%d]\n", i, j);
        dump_matrix_dr("LACORE: A after", A_lacore, N, N);
        dump_matrix_dr("GSL: A after", A, N, N);
        return;
      }
    }
  }
  printf("matrix correct\n");

  if(DEBUG){
    dump_matrix_dr("LACORE: A after", A_lacore, N, N);
    dump_matrix_dr("GSL: A after", A, N, N);
  }

  free_matrix_d(A_lacore);
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
  IDX tmp_scratch_size;

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
    else if(sscanf(argv[i], "--scratch_size=%llu", &tmp_scratch_size) == 1){
      if (tmp_scratch_size == 0) {
        printf("invalid log2 scratch_size");
        exit(0);
      }
      SCRATCH_SIZE = (1 << tmp_scratch_size);
      printf("[ptrans]: scratch_size is %llu\n", SCRATCH_SIZE);
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //-------------------------------------------------------------------------
  if (SCRATCH_SIZE == 0) {
    printf("you must specify scratch_size in bytes\n");
    exit(0);
  }

  //---------------------------------------------------------------------------
  //just to kinda randomize the matrices up
  srand(seed);

  //---------------------------------------------------------------------------

  //set breakpoint here, then make command window huge

  for(IDX NMP=nmp_start; NMP <= nmp_end; NMP *= NMP_SCALE){
    do_test(NMP);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
