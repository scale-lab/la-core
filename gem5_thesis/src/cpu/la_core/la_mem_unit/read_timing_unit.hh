//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_LA_MEM_UNIT_READ_TIMING__
#define __CPU_LA_CORE_LA_MEM_UNIT_READ_TIMING__

#include <cstdint>
#include <functional>
#include <queue>

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"
#include "params/LAMemUnitReadTiming.hh"
#include "sim/ticked_object.hh"

//=============================================================================
// LACore Read Timing Memory Unit Module
//=============================================================================

class LAMemUnitReadTiming : public TickedObject
{
  public: 
    LAMemUnitReadTiming(LAMemUnitReadTimingParams *p);
    ~LAMemUnitReadTiming();

    // overrides
    void evaluate() override;
    void regStats() override;

    void initialize(LACount count, LAReg * reg, LACoreExecContext * xc, 
        std::function<void(uint8_t*,uint8_t,bool)> on_item_load);

  private:
    //set by params
    const uint8_t channel;
    const uint64_t cacheLineSize;
    const uint64_t scratchLineSize;

    volatile bool done;
    std::function<bool(void)> readFunction;

    //modified by readFunction closure over time
    LACount vecIndex;

    //persistent sparse fields read by closures over time
    LACount spvIndex;

    bool gotIdxValCur;
    bool gotIdxValNxt;
    LASpvCount idxValCur;
    LASpvCount idxValNxt;

    LASpvCount jdxOffset;
    bool gotJdxVal;
    LASpvCount jdxVal;

    bool gotDataVal;
    uint8_t *dataVal;

    //local cache-like buffers
    Addr spvCachedIdxLineAddr;
    LASpvCount *spvCachedIdxLine;
    bool spvSentIdxReq;

    Addr spvCachedJdxLineAddr;
    LASpvCount *spvCachedJdxLine;
    bool spvSentJdxReq;

    Addr spvCachedDataLineAddr;
    uint8_t *spvCachedDataLine;
    bool spvSentDataReq;
};

#endif //__CPU_LA_CORE_LA_MEM_UNIT_READ_TIMING__
