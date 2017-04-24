//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// LACore types
//
// NOTE: everything is static to allow this to be a header-only library,
//       so we don't have to link a *.c file just to have a single definition
//       for 'NumLAReg'
//==========================================================================

#ifndef __LA_CORE_LA_CORE_H__
#define __LA_CORE_LA_CORE_H__

#include <stdint.h>

static const   uint8_t  NumLAReg = 8;

typedef uint8_t  LaRegIdx;

typedef uint64_t LaAddr;
typedef uint64_t LaCount;

typedef int32_t  LaVecStride;
typedef uint32_t LaVecCount;
typedef int32_t  LaVecSkip;

#endif //__LA_CORE_LA_CORE_H__
