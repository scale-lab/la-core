//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// NOTE: there is literally no way to use the LACore features for
//       this benchmark, you should just benchmark the RISC-V scalar core
//==========================================================================

#include "random_access/drandom_access_la_core.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//==========================================================================

//Taken from HPCC RandomAccess source code
static const uint64_t LCG_MUL64 = 6364136223846793005ULL;
static const uint64_t LCG_ADD64 = 1;

//Taken from HPCC RandomAccess source code
// ai_next = ((ai * mul) + add) mod (2^64)
//static inline uint64_t LCG_NEXT(uint64_t ai)
//{
//  return (ai * LCG_MUL64) + LCG_ADD64;
//}

void drandom_access_la_core(uint64_t *T, uint64_t size, uint64_t log2_size)
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
