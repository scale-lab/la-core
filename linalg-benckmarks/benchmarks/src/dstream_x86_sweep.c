//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// x86 implementation of the HPCC STREAM benchmark
// http://www.cs.virginia.edu/~mccalpin/papers/bandwidth/node2.html
// http://www.cs.virginia.edu/stream/ref.html
//
// For bandwidth, using the 2st of the 3 methods, counting reads and writes 
// as separate transfers.
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

//==========================================================================

static const IDX VEC_MIN       = (1 << 16);
static const IDX VEC_MAX       = (1 << 20);
static const IDX VEC_SCALE     = 2;

static const double q          = 2.5;

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

void do_copy(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A         = create_matrix_dr(size, 1, NO_FILL);
  double *B         = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  for(IDX i=0; i<size; i+=1) {
    A[i] = B[i];
  }
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_COPY_FLOP(size);
  double bytes = STREAM_COPY_DP_BYTES(size);
  printf("COPY  SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
}


void do_scale(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A         = create_matrix_dr(size, 1, NO_FILL);
  double *B         = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  for(IDX i=0; i<size; i+=1) {
    A[i] = B[i]*q;
  }
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_SCALE_FLOP(size);
  double bytes = STREAM_SCALE_DP_BYTES(size);
  printf("SCALE SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
}


void do_sum(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A         = create_matrix_dr(size, 1, NO_FILL);
  double *B         = create_matrix_dr(size, 1, FILL);
  double *C         = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  for(IDX i=0; i<size; i+=1) {
    A[i] = B[i]+C[i];
  }
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_SUM_FLOP(size);
  double bytes = STREAM_SUM_DP_BYTES(size);
  printf("SUM   SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(C);
}


void do_triad(IDX size)
{
  //--------------------------- CONFIG -------------------------------
  double *A         = create_matrix_dr(size, 1, NO_FILL);
  double *B         = create_matrix_dr(size, 1, FILL);
  double *C         = create_matrix_dr(size, 1, FILL);

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  for(IDX i=0; i<size; i+=1) {
    A[i] = B[i]+q*C[i];
  }
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double flop = STREAM_TRIAD_FLOP(size);
  double bytes = STREAM_TRIAD_DP_BYTES(size);
  printf("TRIAD SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 10.3f MB/s | ", bytes/(duration*1e6));
  printf("% 10.3f MFLOP/s\n", flop/(duration*1e6));

  //--------------------------- CLEANUP ----------------------------------
  free_matrix_d(A);
  free_matrix_d(B);
  free_matrix_d(C);
}

//==========================================================================
// MAIN
//==========================================================================

int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  unsigned seed = time(NULL);
  IDX size_start = VEC_MIN;
  IDX size_end = VEC_MAX;

  unsigned tmp_seed;
  IDX tmp_size;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--seed=%u", &tmp_seed) == 1){
      seed = tmp_seed;
    }
    else if(sscanf(argv[i], "--size=%u", &tmp_size) == 1){
      size_start = tmp_size;
      size_end = tmp_size;
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
  for(IDX SIZE=size_start; SIZE <= size_end; SIZE *= VEC_SCALE){
    do_copy(SIZE);
    do_scale(SIZE);
    do_sum(SIZE);
    do_triad(SIZE);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}

