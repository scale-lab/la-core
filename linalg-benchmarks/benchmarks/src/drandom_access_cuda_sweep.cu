//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// x86 implementation of the HPCC FFT benchmark
// Naive Algorithm pretty much straight from CLRS Algorithms textbook
//=========================================================================

#include <stdbool.h>
#include <string.h>
#include <time.h>

extern "C" {
#include "get_real_time.h"
#include "matrix_types.h"
#include "matrix_utilities.h"
#include "util.h"
}

//==========================================================================

static const IDX TABLE_MIN      = (1 << 4);
static const IDX TABLE_MAX      = (1 << 16);
static const IDX TABLE_SCALE    = 2;

//==========================================================================
// TEST CONFIGURATION, VERIFICATION AND TIMING
//==========================================================================

//Taken from HPCC RandomAccess source code
static const uint64_t LCG_MUL64 = 6364136223846793005ULL;
static const uint64_t LCG_ADD64 = 1;

//Taken from HPCC RandomAccess source code
// ai_next = ((ai * mul) + add) mod (2^64)
static uint64_t LCG_NEXT(uint64_t ai)
{
  return (ai * LCG_MUL64) + LCG_ADD64;
}

//static void drandom_access_inner(uint64_t *T, uint64_t size,
//  uint64_t log2_size)
//{
//  uint64_t seed = LCG_NEXT(1);
//  uint64_t count = 4*size;
//
//  for(uint64_t i=0; i<count; ++i) {
//    uint64_t offset = (seed >> (64 - log2_size));
//    T[offset] ^= seed;
//    seed = LCG_NEXT(seed);
//  }
//}

static void drandom_access_inner(uint64_t *T, uint64_t size, uint64_t log2_size)
{
  uint64_t seed = 1;//LCG_NEXT(1);
  uint64_t count = 4*size;
  uint64_t top_bits = 64 - log2_size;

  for(uint64_t i=0; i<count; i+=8) {
    //get the next 4 seeds
    uint64_t seed1 = (seed  * LCG_MUL64) + LCG_ADD64;
    uint64_t seed2 = (seed1 * LCG_MUL64) + LCG_ADD64;
    uint64_t seed3 = (seed2 * LCG_MUL64) + LCG_ADD64;
    uint64_t seed4 = (seed3 * LCG_MUL64) + LCG_ADD64;
    uint64_t seed5 = (seed4 * LCG_MUL64) + LCG_ADD64;
    uint64_t seed6 = (seed5 * LCG_MUL64) + LCG_ADD64;
    uint64_t seed7 = (seed6 * LCG_MUL64) + LCG_ADD64;
    uint64_t seed8 = (seed7 * LCG_MUL64) + LCG_ADD64;

    //get the next 4 offsets
    uint64_t offset1 = (seed1 >> top_bits);
    uint64_t offset2 = (seed2 >> top_bits);
    uint64_t offset3 = (seed3 >> top_bits);
    uint64_t offset4 = (seed4 >> top_bits);
    uint64_t offset5 = (seed5 >> top_bits);
    uint64_t offset6 = (seed6 >> top_bits);
    uint64_t offset7 = (seed7 >> top_bits);
    uint64_t offset8 = (seed8 >> top_bits);

    //update the next 4 table entries
    T[offset1] ^= seed1;
    T[offset2] ^= seed2;
    T[offset3] ^= seed3;
    T[offset4] ^= seed4;
    T[offset5] ^= seed5;
    T[offset6] ^= seed6;
    T[offset7] ^= seed7;
    T[offset8] ^= seed8;

    //update seed
    seed = seed8;
  }
}


void do_random_access(IDX log_size)
{
  uint64_t size = (1 << log_size);
  uint64_t *T = (uint64_t *)malloc((1 << log_size)*sizeof(uint64_t));

  //---------------------------- RUN ------------------------------------
  double start = get_real_time();
  drandom_access_inner(T, size, log_size);
  double duration = get_real_time() - start;

  //--------------------------- REPORT -----------------------------------
  double gup = RANDOM_ACCESS_GUP(size);
  printf("SIZE=%10lu | took % 10.6f secs | ", size, duration);
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
