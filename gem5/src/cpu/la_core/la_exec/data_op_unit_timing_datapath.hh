/*
 * Copyright (c) 2017 Brown University
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Samuel Steffl
 */

#ifndef __CPU_LA_CORE_LA_EXEC_DATA_OP_UNIT_TIMING_DATAPATH_HH__
#define __CPU_LA_CORE_LA_EXEC_DATA_OP_UNIT_TIMING_DATAPATH_HH__

#include <cstdint>
#include <functional>

#include "cpu/la_core/la_exec/data_op_unit_timing.hh"
#include "cpu/la_core/la_insn.hh"
#include "params/LACoreDataOpUnitTimingDatapath.hh"
#include "sim/ticked_object.hh"

class LACoreDataOpUnitTiming;

class LACoreDataOpUnitTimingDatapath : public TickedObject
{
private:
    class TimedFunc {
      public:
        TimedFunc(uint64_t cyclesLeft, std::function<void(void)> execute) :
            cyclesLeft(cyclesLeft), execute(execute) {}
        ~TimedFunc() {}

        uint64_t cyclesLeft;
        std::function<void(void)> execute;
    };

public:
    LACoreDataOpUnitTimingDatapath(LACoreDataOpUnitTimingDatapathParams *p);
    ~LACoreDataOpUnitTimingDatapath();

    void startTicking(LACoreDataOpUnitTiming& data_op_unit, LAInsn& insn, 
        uint64_t src_count, uint64_t dst_count, uint64_t mst_count,
        bool A_dp, bool B_dp, bool C_dp, bool D_dp,
        std::function<void(uint8_t*,uint8_t)> data_callback);
    void stopTicking();

    //overrides
    void regStats() override;
    void evaluate() override;

private:
    //configuration from python object
    const uint64_t addLatencySp;
    const uint64_t addLatencyDp;
    const uint64_t mulLatencySp;
    const uint64_t mulLatencyDp;
    const uint64_t divLatencySp;
    const uint64_t divLatencyDp;
    //const uint64_t vecFuncLatency;
    //const uint64_t reduceFuncLatency;
    //const uint64_t simdWidth;
    const uint64_t simdWidthDp;
    const uint64_t vecNodes;

    //state passed in for current instruction
    LACoreDataOpUnitTiming* dataOpUnit;
    LAInsn* insn;
    uint64_t srcCount;
    uint64_t dstCount;
    uint64_t mstCount;
    bool A_dp;
    bool B_dp;
    bool C_dp;
    bool D_dp;
    std::function<void(uint8_t*,uint8_t)> dataCallback;

    //internal state for current instruction
    std::deque<TimedFunc *> vecFuncs;
    std::deque<TimedFunc *> reduceFuncs;
    uint64_t curSrcCount;
    uint64_t curDstCount;
    uint64_t curMstCount;
    double accumDp;
    float  accumSp;
    uint64_t SRCA_SIZE;
    uint64_t SRCB_SIZE;
    uint64_t SRCC_SIZE;
    uint64_t DST_SIZE;
};

#endif //__CPU_LA_CORE_LA_EXEC_DATA_OP_UNIT_TIMING_DATAPATH_HH__
