//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//
// RISC-V LACore implementation of the HPCC STREAM benchmark
// http://www.cs.virginia.edu/~mccalpin/papers/bandwidth/node2.html
// http://www.cs.virginia.edu/stream/ref.html
//==========================================================================

#ifndef __DRANDOM_ACCESS_LA_CORE_H__
#define __DRANDOM_ACCESS_LA_CORE_H__

#include <stdint.h>

#include "matrix_types.h"

void drandom_access_la_core(uint64_t *T, uint64_t size, uint64_t log2_size);


#endif // __DRANDOM_ACCESS_LA_CORE_H__
