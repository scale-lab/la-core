//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of the HPCC FFT benchmark
// Naive Algorithm pretty much straight from CLRS Algorithms textbook
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "random_access/drandom_access_la_core.h"
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"

//==========================================================================

static const IDX TABLE_MIN      = (1 << 4);
static const IDX TABLE_MAX      = (1 << 16);
static const IDX TABLE_SCALE    = 2;

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

void do_random_access(IDX log_size)
{
  uint64_t size = (1 << log_size);
  uint64_t *T = (uint64_t *)malloc((1 << log_size)*sizeof(uint64_t));

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  drandom_access_la_core(T, size, log_size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double gup = RANDOM_ACCESS_GUP(size);
  printf("SIZE=%10d | took % 10.6f secs | ", size, duration);
  printf("% 12.3f GUPS/s\n", gup/duration);

  //--------------------------- CLEANUP ----------------------------------
  free(T);
}

//==========================================================================
// MAIN
//==========================================================================

int main(int argc, char **argv)
{
  //---------------------------------------------------------------------------
  IDX log_size_start = TABLE_MIN;
  IDX log_size_end = TABLE_MAX;

  IDX tmp_log_size;

  //---------------------------------------------------------------------------
  //parse args
  for(int i=1; i<argc; ++i){
    if(sscanf(argv[i], "--log_size=%u", &tmp_log_size) == 1){
      log_size_start = tmp_log_size;
      log_size_end = tmp_log_size;
    }
    else {
      printf("unrecognized argument: %s\n", argv[i]);
      exit(0);
    }
  }

  //---------------------------------------------------------------------------

  //set breakpoint here, then make command window huge
  for(IDX SIZE=log_size_start; SIZE <= log_size_end; SIZE *= TABLE_SCALE){
    do_random_access(SIZE);
    PRINT("\n");
  }

  //set breakpoint here, then analyze results
  return 0;
}
