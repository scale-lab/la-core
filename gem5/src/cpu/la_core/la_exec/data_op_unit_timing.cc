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

#include "cpu/la_core/la_exec/data_op_unit_timing.hh"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_exec/data_op_unit.hh"
#include "cpu/la_core/la_exec/data_op_unit_timing_datapath.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_mem_unit/read_timing_unit.hh"
#include "cpu/la_core/la_mem_unit/write_timing_unit.hh"
#include "params/LACoreDataOpUnitTiming.hh"
#include "sim/faults.hh"
#include "sim/sim_object.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreDataOpUnitTiming::LACoreDataOpUnitTiming(
    const LACoreDataOpUnitTimingParams *p) :
    LACoreDataOpUnit(p), dataPath(p->dataPath)
{
}

LACoreDataOpUnitTiming::~LACoreDataOpUnitTiming()
{
}

//=============================================================================
// interface functions
//=============================================================================

bool 
LACoreDataOpUnitTiming::isOccupied()
{
    return occupied;
}

void 
LACoreDataOpUnitTiming::issue(LAInsn& insn, LACoreExecContext *xc,
    std::function<void(Fault fault)> done_callback)
{
    assert(!occupied);
    occupied = true;

    bool is_reduce = (insn.isLADataReduce());
    LAReg * srcA = &(xc->getLAReg(insn.lra()));
    LAReg * srcB = &(xc->getLAReg(insn.lrb()));
    LAReg * srcC = &(xc->getLAReg(insn.lrc()));
    LAReg * dst = &(xc->getLAReg(insn.lrd()));
    LACount count = xc->readIntRegFlat(insn.lrxa());

    //no-ops are fine
    if (count == 0) {
        occupied = false;
        done_callback(NoFault);
        return;
    }

    //if D is scalar, must be single-out, single-stream
    assert(dst->vector() || count == 1 ||
      (insn.isLADataReduce() && !insn.mst()));

    bool A_dp = srcA->dp();
    bool B_dp = srcB->dp();
    bool C_dp = srcC->dp();
    bool D_dp = dst->dp();

    uint64_t SRCA_SIZE = A_dp ? sizeof(double) : sizeof(float);
    uint64_t SRCB_SIZE = B_dp ? sizeof(double) : sizeof(float);
    uint64_t SRCC_SIZE = C_dp ? sizeof(double) : sizeof(float);
    uint64_t DST_SIZE  = D_dp ? sizeof(double) : sizeof(float);

    LACount Acount = (srcA->vector() ? 
                      (srcA->sparse() ? srcA->jdxCnt() : srcA->vecCount()) :
                      0);
    LACount Bcount = (srcB->vector() ? 
                      (srcB->sparse() ? srcB->jdxCnt() : srcB->vecCount()) :
                      0);
    LACount Ccount = (srcC->vector() ? 
                      (srcC->sparse() ? srcC->jdxCnt() : srcC->vecCount()) :
                      0);

    //multi-stream partition count
    LACount mst_count = 0;

    //how many items will get written to dst by the end of the operation
    LACount dst_count = count;

    //adjust D output bytes if single-out or single-out/single-stream
    if (is_reduce) {
        dst_count /= count;
        if (insn.mst()) {
            mst_count = Acount;

            assert(!Bcount || !mst_count || (Bcount == mst_count));
            mst_count = (mst_count ? mst_count : Bcount);

            assert(!Ccount || !mst_count || (Ccount == mst_count));
            mst_count = (mst_count ? mst_count : Ccount);

            assert(mst_count && (count % mst_count == 0));
            dst_count = (count/mst_count);
            //dst_count *= mst_count;
        }
    }

    //START THE DATAPATH
    dataPath->startTicking(*this, insn, count, dst_count, mst_count,
        A_dp, B_dp, C_dp, D_dp, [DST_SIZE,this](uint8_t *data, uint8_t size)
    {
        assert(size == DST_SIZE);
        this->dstWriter->queueData(data);
    });

    //NOTE: need to initialize the writer BEFORE the readers!
    this->Dread = 0;

    dstWriter->initialize(dst_count, dst, xc, 
        [done_callback,dst_count,this](bool done) 
    {
        ++Dread;
        if(done) {
            assert(this->Dread == dst_count);
            this->occupied = false;
            this->dataPath->stopTicking();
            done_callback(NoFault);
        }
    });

    this->Aread = 0;
    this->Bread = 0;
    this->Cread = 0;

    this->AdataQ.clear();
    this->BdataQ.clear();
    this->CdataQ.clear();

    srcAReader->initialize(count, srcA, xc, [SRCA_SIZE,count,this]
        (uint8_t*data, uint8_t size, bool done) 
    {
        assert(size == SRCA_SIZE);
        uint8_t *ndata = new uint8_t[SRCA_SIZE];
        memcpy(ndata, data, SRCA_SIZE);
        this->AdataQ.push_back(ndata);
        ++this->Aread;
        assert(!done || (this->Aread == count));
    });
    srcBReader->initialize(count, srcB, xc, [SRCB_SIZE,count,this]
        (uint8_t*data, uint8_t size, bool done) 
    {
        assert(size == SRCB_SIZE);
        uint8_t *ndata = new uint8_t[SRCB_SIZE];
        memcpy(ndata, data, SRCB_SIZE);
        this->BdataQ.push_back(ndata);
        ++this->Bread;
        assert(!done || (this->Bread == count));
    });
    srcCReader->initialize(count, srcC, xc, [SRCC_SIZE,count,this]
        (uint8_t*data, uint8_t size, bool done) 
    {
        assert(size == SRCC_SIZE);
        uint8_t *ndata = new uint8_t[SRCC_SIZE];
        memcpy(ndata, data, SRCC_SIZE);
        this->CdataQ.push_back(ndata);
        ++this->Cread;
        assert(!done || (this->Cread == count));
    });
}

//=============================================================================
// LACoreDataOpUnitTimingParams factory
//=============================================================================

LACoreDataOpUnitTiming *
LACoreDataOpUnitTimingParams::create()
{
    return new LACoreDataOpUnitTiming(this);
}

