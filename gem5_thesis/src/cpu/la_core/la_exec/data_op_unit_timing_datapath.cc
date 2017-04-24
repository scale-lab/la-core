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

#include "cpu/la_core/la_exec/data_op_unit_timing_datapath.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <queue>

#include "cpu/la_core/la_exec/data_op_unit_timing.hh"
#include "cpu/la_core/la_insn.hh"
#include "params/LACoreDataOpUnitTimingDatapath.hh"
#include "sim/ticked_object.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreDataOpUnitTimingDatapath::LACoreDataOpUnitTimingDatapath(
    LACoreDataOpUnitTimingDatapathParams *p) :
    TickedObject(p),
    addLatencySp(p->addLatencySp), addLatencyDp(p->addLatencyDp),
    mulLatencySp(p->mulLatencySp), mulLatencyDp(p->mulLatencyDp),
    divLatencySp(p->divLatencySp), divLatencyDp(p->divLatencyDp),
    //vecFuncLatency(p->vecFuncLatency), reduceFuncLatency(p->reduceFuncLatency)
    simdWidthDp(p->simdWidthDp), vecNodes(p->vecNodes)
{
}

LACoreDataOpUnitTimingDatapath::~LACoreDataOpUnitTimingDatapath()
{
}

//=============================================================================
// Public Interface Functions
//=============================================================================

void
LACoreDataOpUnitTimingDatapath::startTicking(
    LACoreDataOpUnitTiming& data_op_unit, LAInsn& insn, 
    uint64_t src_count, uint64_t dst_count, uint64_t mst_count,
    bool A_dp, bool B_dp, bool C_dp, bool D_dp,
    std::function<void(uint8_t*,uint8_t)> data_callback)
{
    assert(!running);

    //copy over the configuration inputs
    this->dataOpUnit = &data_op_unit;
    this->insn = &insn;
    this->srcCount = src_count;
    this->dstCount = dst_count;
    this->mstCount = mst_count;
    this->A_dp = A_dp;
    this->B_dp = B_dp;
    this->C_dp = C_dp;
    this->D_dp = D_dp;
    this->dataCallback = data_callback;

    //reset all state
    vecFuncs.clear();
    reduceFuncs.clear();
    curSrcCount = 0;
    curDstCount = 0;
    curMstCount = 0;
    accumDp = 0.0;
    accumSp = 0.0;
    SRCA_SIZE = A_dp ? sizeof(double) : sizeof(float);
    SRCB_SIZE = B_dp ? sizeof(double) : sizeof(float);
    SRCC_SIZE = C_dp ? sizeof(double) : sizeof(float);
    DST_SIZE = D_dp ? sizeof(double) : sizeof(float);

    //start
    start();
}


void
LACoreDataOpUnitTimingDatapath::stopTicking()
{
    assert(running);
    stop();
}

//=============================================================================
// Overrides
//=============================================================================

void
LACoreDataOpUnitTimingDatapath::regStats()
{
  TickedObject::regStats();
  ClockedObject::regStats();
}

void
LACoreDataOpUnitTimingDatapath::evaluate()
{
    assert(running);

    //move Datapath forward before looking at inputs and stuff
    for (auto reduceFunc : reduceFuncs) {
        --reduceFunc->cyclesLeft;
    }
    for (auto vecFunc : vecFuncs) {
        --vecFunc->cyclesLeft;
    }

    //after the configured latencies for the datapath, commit the operation
    if (reduceFuncs.size()) {
      TimedFunc * reduceFunc = reduceFuncs.front();
      if (reduceFunc->cyclesLeft == 0) {
          reduceFuncs.pop_front();
          reduceFunc->execute();
          delete reduceFunc;
      }
    }
    if (vecFuncs.size()) {
      TimedFunc * vecFunc = vecFuncs.front();
      if (vecFunc->cyclesLeft == 0) {
          vecFuncs.pop_front();
          vecFunc->execute();
          delete vecFunc;
      }
    }


    //get data into SIMD vectors
    //don't queue up more execution if enough data is not ready
    //we can fit 2x the items in buffer if 32-bit instead of 64-bit!
    uint64_t max_simd_items = simdWidthDp*(sizeof(double)/DST_SIZE)*vecNodes;

    //NOTE: i changed this!
    //uint64_t simd_size = std::min(max_simd_items, srcCount-curSrcCount);
    uint64_t simd_size = std::min(
                          std::min(max_simd_items, srcCount-curSrcCount),
                          std::min(dataOpUnit->AdataQ.size(),
                            std::min(dataOpUnit->BdataQ.size(),
                              dataOpUnit->CdataQ.size())));

    if (//(dataOpUnit->AdataQ.size() < simd_size) ||
        //(dataOpUnit->BdataQ.size() < simd_size) ||
        //(dataOpUnit->CdataQ.size() < simd_size) ||
        (simd_size == 0))
    {
        return;
    }

    uint8_t * Adata = (uint8_t *)malloc(simd_size*SRCA_SIZE);
    uint8_t * Bdata = (uint8_t *)malloc(simd_size*SRCB_SIZE);
    uint8_t * Cdata = (uint8_t *)malloc(simd_size*SRCC_SIZE);

    for(uint64_t i=0; i<simd_size; ++i){
        uint8_t *Aitem = dataOpUnit->AdataQ.front();
        uint8_t *Bitem = dataOpUnit->BdataQ.front();
        uint8_t *Citem = dataOpUnit->CdataQ.front();

        memcpy(Adata+(i*SRCA_SIZE), Aitem, SRCA_SIZE);
        memcpy(Bdata+(i*SRCB_SIZE), Bitem, SRCB_SIZE);
        memcpy(Cdata+(i*SRCC_SIZE), Citem, SRCC_SIZE);

        dataOpUnit->AdataQ.pop_front();
        dataOpUnit->BdataQ.pop_front();
        dataOpUnit->CdataQ.pop_front();

        delete[] Aitem;
        delete[] Bitem;
        delete[] Citem;
    }
    //NOTE: need to track how many we've read in case the last SIMD batch is
    //      partially full
    curSrcCount += simd_size;

    //calculate the latencies for the current operation
    uint64_t vecFuncLatency;
    uint64_t reduceFuncLatency;
    if(D_dp) {
        vecFuncLatency =
            addLatencyDp + (insn->dv() ? divLatencyDp : mulLatencyDp);
        reduceFuncLatency = addLatencyDp*(uint64_t)log2((float)vecNodes);
    } else {
        vecFuncLatency =
            addLatencySp + (insn->dv() ? divLatencySp : mulLatencySp);
        reduceFuncLatency = addLatencySp*(uint64_t)log2((float)vecNodes);
    }

    //queue up the vecNode results for the given latency
    vecFuncs.push_back(new TimedFunc(vecFuncLatency,
        [this,reduceFuncLatency,simd_size,Adata,Bdata,Cdata](){

        //create data result buffer
        uint8_t * Ddata = (uint8_t *)malloc(simd_size*DST_SIZE);

        //do the SIMD vector operation
        for(LACount i=0; i<simd_size; ++i){
            if(D_dp){
                //coerse unit
                double Aitem = 
                    (double)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
                double Bitem = 
                    (double)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
                double Citem = 
                    (double)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

                //(A+-B)*/C or (A*/B)+-C
                //t2 == 'terms' on top, not top-2 anymore!
                double top   =insn->t2()? Aitem+(insn->su() ? -Bitem  : Bitem)
                                        : Aitem*(insn->dv() ? 1/Bitem : Bitem);
                double Ditem =insn->t2()? top*  (insn->dv() ? 1/Citem : Citem)
                                        : top+  (insn->su() ? -Citem  : Citem);

                memcpy(Ddata+(i*DST_SIZE), (uint8_t*)&Ditem, DST_SIZE);
            } else {
                //coerse unit
                float Aitem = 
                    (float)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
                float Bitem = 
                    (float)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
                float Citem = 
                    (float)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

                //(A+-B)*/C or (A*/B)+-C
                //t2 == 'terms' on top, not top-2 anymore!
                float top   = insn->t2()? Aitem+(insn->su() ? -Bitem  : Bitem)
                                        : Aitem*(insn->dv() ? 1/Bitem : Bitem);
                float Ditem = insn->t2()? top*  (insn->dv() ? 1/Citem : Citem)
                                        : top+  (insn->su() ? -Citem  : Citem);

                memcpy(Ddata+(i*DST_SIZE), (uint8_t*)&Ditem, DST_SIZE);
            }
        }

        //don't need these anymore
        delete[] Adata;
        delete[] Bdata;
        delete[] Cdata;

        //if reducing, need to wait some more time, otherwise done!
        if (insn->isLADataReduce()) {
            reduceFuncs.push_back(new TimedFunc(reduceFuncLatency,
                [this,simd_size,Ddata]{

                for (LACount i=0; i < simd_size; ++i) {
                    if (D_dp) {
                        double Ditem = ((double*)Ddata)[i];
                        switch(insn->rdct()){
                            case LAReduce::Min:
                                accumDp  = std::min(accumDp, Ditem); break;
                            case LAReduce::Max: 
                                accumDp  = std::max(accumDp, Ditem); break;
                            case LAReduce::Sum: 
                                accumDp += Ditem;                    break;
                            default:            
                                assert(false);                       break;
                        }
                        //check if we hit a multi-reduce checkpoint!
                        if (insn->mst() && ((curMstCount+1) % mstCount == 0)) {
                            uint8_t *ndata = new uint8_t[DST_SIZE];
                            memcpy(ndata, (uint8_t*)&accumDp, DST_SIZE);
                            dataCallback(ndata, DST_SIZE);
                            accumDp = 0.0;
                            ++curDstCount;
                        }
                    } else {
                        float Ditem = ((float*)Ddata)[i];
                        switch(insn->rdct()){
                            case LAReduce::Min:
                                accumSp  = std::min(accumSp, Ditem); break;
                            case LAReduce::Max: 
                                accumSp  = std::max(accumSp, Ditem); break;
                            case LAReduce::Sum: 
                                accumSp += Ditem;                    break;
                            default:            
                                assert(false);                       break;
                        }
                        if (insn->mst() && ((curMstCount+1) % mstCount == 0)) {
                            uint8_t *ndata = new uint8_t[DST_SIZE];
                            memcpy(ndata, (uint8_t*)&accumSp, DST_SIZE);
                            dataCallback(ndata, DST_SIZE);
                            accumSp = 0.0;
                            ++curDstCount;
                        }
                    }
                    ++curMstCount;
                }
                if (!insn->mst() && (curMstCount == srcCount)) {
                    assert(curDstCount == 0 && dstCount == 1);
                    uint8_t *ndata = new uint8_t[DST_SIZE];
                    if (D_dp) {
                        memcpy(ndata, (uint8_t*)&accumDp, DST_SIZE);
                    } else {
                        memcpy(ndata, (uint8_t*)&accumSp, DST_SIZE);
                    }
                    dataCallback(ndata, DST_SIZE);
                    ++curDstCount;
                }

                //done with this now
                delete [] Ddata;
            }));
        } else {
            //push all the data to the dataOpUnit since not reducing
            for (LACount i=0; i<simd_size; ++i) {
                uint8_t *ndata = new uint8_t[DST_SIZE];
                memcpy(ndata, Ddata+(i*DST_SIZE), DST_SIZE);
                dataCallback(ndata, DST_SIZE);
                ++curDstCount;
            }
            delete [] Ddata;
        }
    }));
}


//=============================================================================
// LACoreDataOpUnitTimingDatapathParams factory
//=============================================================================

LACoreDataOpUnitTimingDatapath *
LACoreDataOpUnitTimingDatapathParams::create()
{
    return new LACoreDataOpUnitTimingDatapath(this);
}

