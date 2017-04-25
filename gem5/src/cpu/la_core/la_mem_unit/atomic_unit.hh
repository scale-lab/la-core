//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_LA_MEM_UNIT_ATOMIC__
#define __CPU_LA_CORE_LA_MEM_UNIT_ATOMIC__

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"

//=============================================================================
// LACore LAMemUnitAtomic (NOT a SimObject!)
//=============================================================================

//forward decls
class LACoreExecContext;
class LAReg;

class LAMemUnitAtomic
{
  public: 
    LAMemUnitAtomic();
    ~LAMemUnitAtomic();

    //read and write the FULL vector/scalar into/out-of 'data' buffer in 1 call
    void readAtomic(uint8_t *data, LACount count, LAReg * reg,
        LACoreExecContext * xc);
    void writeAtomic(uint8_t *data, LACount count, LAReg * reg,
        LACoreExecContext * xc);
  
  private:
    LASpvCount spv_read_idx_jdx_array(LAReg *reg, LAAddr array_ptr, 
        LACount offset, LACoreExecContext * xc);
    float spv_read_sp_array(LAReg *reg, LAAddr array_ptr, 
        LACount offset, LACoreExecContext * xc);
    double spv_read_dp_array(LAReg *reg, LAAddr array_ptr, 
        LACount offset, LACoreExecContext * xc);
};

#endif //__CPU_LA_CORE_LA_MEM_UNIT_ATOMIC__
