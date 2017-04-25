//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_SCRATCHPAD_ATOMIC__
#define __CPU_LA_CORE_SCRATCHPAD_ATOMIC__

#include <cstdint>

#include "cpu/la_core/la_types.hh"
#include "sim/faults.hh"

//=============================================================================
// LACore Scratchpad Memory 
//=============================================================================

class LAScratchAtomic
{
public:
  LAScratchAtomic(uint64_t size);
  ~LAScratchAtomic();

  Fault readAtomic(LAAddr addr, uint8_t* data, uint64_t _size);
  Fault writeAtomic(LAAddr addr, uint8_t* data, uint64_t _size);

private:
  uint8_t *mem;
  uint64_t size;
};

#endif //__CPU_LA_CORE_SCRATCHPAD_ATOMIC__
