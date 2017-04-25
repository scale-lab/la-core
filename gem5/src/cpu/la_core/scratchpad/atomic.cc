//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "cpu/la_core/scratchpad/atomic.hh"

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include "cpu/la_core/la_types.hh"
#include "sim/faults.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LAScratchAtomic::LAScratchAtomic(uint64_t size) :
  size(size)
{
  assert((mem = (uint8_t *)malloc(sizeof(uint8_t)*size)) != nullptr);
}

LAScratchAtomic::~LAScratchAtomic()
{
  free(mem);
}

//=============================================================================
// public interface functions
//=============================================================================

Fault
LAScratchAtomic::readAtomic(LAAddr addr, uint8_t* data, uint64_t len)
{
  assert(addr + len <= size);
  memcpy(data, mem+addr, len);
  return NoFault;
}

Fault
LAScratchAtomic::writeAtomic(LAAddr addr, uint8_t* data, uint64_t len)
{
  assert(addr + len <= size);
  memcpy(mem+addr, data, len);
  return NoFault;
}
