//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_LA_MEM_UNIT_WRITE_TIMING__
#define __CPU_LA_CORE_LA_MEM_UNIT_WRITE_TIMING__

#include <cstdint>
#include <deque>
#include <functional>

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"
#include "params/LAMemUnitWriteTiming.hh"
#include "sim/ticked_object.hh"

//=============================================================================
// LACore Write Timing Memory Unit Module
//=============================================================================

class LAMemUnitWriteTiming : public TickedObject
{
  public: 
    LAMemUnitWriteTiming(LAMemUnitWriteTimingParams *p);
    ~LAMemUnitWriteTiming();

    // overrides
    void evaluate() override;
    void regStats() override;

    void queueData(uint8_t *data);
    void initialize(LACount count, LAReg * reg, LACoreExecContext * xc, 
        std::function<void(bool)> on_item_store);

  private:
    //set by params
    const uint8_t channel;
    const uint64_t cacheLineSize;
    const uint64_t scratchLineSize;
    
    volatile bool done;
    std::deque<uint8_t *> dataQ;
    std::function<bool(void)> writeFunction;

    //modified by writeFunction closure over time
    LACount vecIndex;
};

#endif //__CPU_LA_CORE_LA_MEM_UNIT_WRITE_TIMING__
