//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __LA_SCRATCH__
#define __LA_SCRATCH__

#include <cstdint>

#include "la_types.h"

namespace la_core {
  
//=============================================================================
// RISC-V LACore Scratchpad Memory 
//=============================================================================

class LAScratch
{
public:
  LAScratch(uint64_t size);
  ~LAScratch();

  uint64_t readUint64(LAAddr addr);
  uint32_t readUint32(LAAddr addr);

  void writeUint64(LAAddr addr, uint64_t data);
  void writeUint32(LAAddr addr, uint32_t data);

private:
  char *mem;
  uint64_t size;
};

} //namespace la_core

#endif //__LA_SCRATCH__