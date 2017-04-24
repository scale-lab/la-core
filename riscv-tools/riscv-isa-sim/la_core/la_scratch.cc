//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "la_scratch.h"
#include "la_types.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>


namespace la_core {

//=============================================================================
// constructor/destructor
//=============================================================================

LAScratch::LAScratch(uint64_t size) :
  size(size)
{
  assert((mem = (char *)malloc(size)) != nullptr);
}

LAScratch::~LAScratch()
{
  free(mem);
}

//=============================================================================
// public interface functions
//=============================================================================

uint64_t 
LAScratch::readUint64(LAAddr addr)
{
  assert(addr + sizeof(uint64_t) <= size);
  return *(uint64_t *)(mem + addr);
}

uint32_t 
LAScratch::readUint32(LAAddr addr)
{
  assert(addr + sizeof(uint32_t) <= size);
  return *(uint32_t *)(mem + addr);
}

void 
LAScratch::writeUint64(LAAddr addr, uint64_t data)
{
  assert(addr + sizeof(uint64_t) <= size);
  *(uint64_t *)(mem + addr) = data;
}

void 
LAScratch::writeUint32(LAAddr addr, uint32_t data)
{
  assert(addr + sizeof(uint32_t) <= size);
  *(uint32_t *)(mem + addr) = data;
}

} //namespace la_core