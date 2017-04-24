//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __LA_MMU__
#define __LA_MMU__

#include <cstdint>

#include "la_scratch.h"
#include "la_types.h"
#include "processor.h"

namespace la_core {

//=============================================================================
// RISC-V LACore MMU
//=============================================================================

//forward decls
class LAScratch;
class LAReg;


class LAMMU
{
public: 
  LAMMU(processor_t*& p, LAScratch * scratch);
  ~LAMMU();

  void read(char *data, LACount count, LAReg * reg);
  void write(char *data, LACount count, LAReg * reg);

private:
  LASpvCount spv_read_idx_jdx_array(LAReg *reg,
    LAAddr array_ptr, LACount offset);
  float spv_read_sp_array(LAReg *reg,
    LAAddr array_ptr, LACount offset);
  double spv_read_dp_array(LAReg *reg,
    LAAddr array_ptr, LACount offset);

  processor_t*& p; //needed to use READ_REG/WRITE_REG
  LAScratch * scratch;
};

} //namespace la_core

#endif //__LA_MMU__
