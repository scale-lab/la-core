//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __LA_TYPES_H__
#define __LA_TYPES_H__

#include <cstdint>

namespace la_core {

//=============================================================================
// RISC-V LACore Constants and Types
//=============================================================================

const uint32_t NumLAReg     = 8;

typedef uint8_t  LARegIdx;

typedef uint64_t LAAddr;
typedef uint64_t LACount;

typedef int32_t  LAVecStride;
typedef uint32_t LAVecCount;
typedef int32_t  LAVecSkip;

typedef uint32_t LASpvCount;

//=============================================================================
// RISC-V LACore CSR flag enumeration
//=============================================================================

typedef enum LACsrFlag
{
  BogusFlag = 0
}
LACsrFlag;

const char NumLACsrFlags = 1;


} //namespace la_core

#endif // __LA_TYPES_H__
