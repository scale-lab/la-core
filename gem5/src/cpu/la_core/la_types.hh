//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_LA_TYPES_H__
#define __CPU_LA_CORE_LA_TYPES_H__

#include <cstdint>

//=============================================================================
// LACore Constants and Types
//=============================================================================

//NOTE: these are now set in the arch/registers and in the python config
//const uint32_t NumLAReg     = 8;
//const uint32_t ScratchSize  = (1 << 16);

typedef uint8_t  LARegIdx;

typedef uint64_t LAAddr;
typedef uint64_t LACount;

typedef int32_t  LAVecStride;
typedef uint32_t LAVecCount;
typedef int32_t  LAVecSkip;

typedef uint32_t LASpvCount;

//=============================================================================
// LACore CSR flag enumeration
//=============================================================================

typedef enum LACsrFlag
{
  BogusFlag = 0
}
LACsrFlag;

const char NumLACsrFlags = 1;


#endif // __CPU_LA_CORE_LA_TYPES_H__