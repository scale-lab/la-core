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

#include "cpu/la_core/la_exec/data_op_unit_atomic.hh"

#include <cstdint>
#include <cstring>
#include <functional>

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_exec/data_op_unit.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_mem_unit/read_timing_unit.hh"
#include "cpu/la_core/la_mem_unit/write_timing_unit.hh"
#include "params/LACoreDataOpUnitAtomic.hh"
#include "sim/faults.hh"
#include "sim/sim_object.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreDataOpUnitAtomic::LACoreDataOpUnitAtomic(
    const LACoreDataOpUnitAtomicParams *p) :
    LACoreDataOpUnit(p)
{
}

LACoreDataOpUnitAtomic::~LACoreDataOpUnitAtomic()
{
}

//=============================================================================
// interface functions
//=============================================================================

bool 
LACoreDataOpUnitAtomic::isOccupied()
{
    return occupied;
}

void 
LACoreDataOpUnitAtomic::issue(LAInsn& insn, LACoreExecContext *xc,
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

//=============================================================================
// START TEMPORARY ATOMIC DATAPATH!
//=============================================================================

    //ATOMIC: allocate the buffers
    uint8_t * Adata = (uint8_t *)malloc(count*SRCA_SIZE);
    uint8_t * Bdata = (uint8_t *)malloc(count*SRCB_SIZE);
    uint8_t * Cdata = (uint8_t *)malloc(count*SRCC_SIZE);
    uint8_t * Ddata = (uint8_t *)malloc(dst_count*DST_SIZE);

    //pass everything by value since lambda will outlive surrounding function
    auto atomicDataPath = [&insn,is_reduce,mst_count,dst_count,DST_SIZE,
        Adata,Bdata,Cdata,Ddata,A_dp,B_dp,C_dp,D_dp,count,this]() -> void
    {
        //used for reduce instructions
        double accum_dp = 0.0;
        float  accum_sp = 0.0;

        for(LACount i=0; i<count; ++i){
            if(D_dp){
                double Aitem = 
                    (double)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
                double Bitem = 
                    (double)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
                double Citem = 
                    (double)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

                //(A+-B)*/C or (A*/B)+-C
                //t2 == 'terms' on top, not top-2 anymore!
                double top   = insn.t2() ? Aitem+(insn.su() ? -Bitem  : Bitem)
                                         : Aitem*(insn.dv() ? 1/Bitem : Bitem);
                double Ditem = insn.t2() ? top*  (insn.dv() ? 1/Citem : Citem)
                                         : top+  (insn.su() ? -Citem  : Citem);
                          
                if(is_reduce){
                    //scalar output
                    switch(insn.rdct()){
                        case LAReduce::Min: 
                            accum_dp  = std::min(accum_dp, Ditem); break;
                        case LAReduce::Max: 
                            accum_dp  = std::max(accum_dp, Ditem); break;
                        case LAReduce::Sum: 
                            accum_dp += Ditem;                     break;
                        default:            
                            assert(false);                         break;
                    }
                    if(insn.mst() && ((i+1) % mst_count == 0)){
                      ((double *)Ddata)[i/ mst_count] = accum_dp;
                      accum_dp = 0;
                    }
                } else {
                    //vector output
                    ((double *)Ddata)[i] = Ditem;
                }
            } else {
                float Aitem = 
                    (float)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
                float Bitem = 
                    (float)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
                float Citem = 
                    (float)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

                //(A+-B)*/C or (A*/B)+-C
                //t2 == 'terms' on top, not top-2 anymore!
                float top   = insn.t2() ? Aitem+(insn.su() ? -Bitem  : Bitem)
                                        : Aitem*(insn.dv() ? 1/Bitem : Bitem);
                float Ditem = insn.t2() ? top*  (insn.dv() ? 1/Citem : Citem)
                                        : top+  (insn.su() ? -Citem  : Citem);
                              
                if(is_reduce){
                    //scalar output
                    switch(insn.rdct()){
                        case LAReduce::Min: 
                            accum_sp  = std::min(accum_sp, Ditem); break;
                        case LAReduce::Max: 
                            accum_sp  = std::max(accum_sp, Ditem); break;
                        case LAReduce::Sum: 
                            accum_sp += Ditem;                     break;
                        default:            
                            assert(false);                         break;
                    }
                    if (insn.mst() && ((i+1) % mst_count == 0)) {
                        ((float *)Ddata)[i/ mst_count] = accum_sp;
                        accum_sp = 0;
                    }
                } else {
                    //vector output
                    ((float *)Ddata)[i] = Ditem;
                }
            }
        }
        if(is_reduce && !insn.mst()){
            if(D_dp){
                ((double *)Ddata)[0] = accum_dp;
            } else {
                ((float *)Ddata)[0] = accum_sp;
            }
        }

        //now atomically write all the data to the dstQ
        for(LACount i=0; i<dst_count; ++i){
            uint8_t *ndata = new uint8_t[DST_SIZE];
            memcpy(ndata, Ddata+(i*DST_SIZE), DST_SIZE);
            dstWriter->queueData(ndata);
        }

        //free the atomic data right now, done with it
        free(Adata);
        free(Bdata);
        free(Cdata);
        free(Ddata);
    };

//=============================================================================
// END TEMPORARILY ATOMIC DATAPATH!
//=============================================================================

    //NOTE: need to initialize the writer BEFORE the reader!
    this->Dread = 0;

    dstWriter->initialize(dst_count, dst, xc, 
        [done_callback,dst_count,this](bool done) 
    {
        ++Dread;
        if(done) {
            assert(this->Dread == dst_count);
            this->occupied = false;
            done_callback(NoFault);
        }
    });

//=============================================================================
// START TEMPORARY ATOMIC MEM-READING!
//=============================================================================

    this->Adone = false;
    this->Bdone = false;
    this->Cdone = false;

    this->Aread = 0;
    this->Bread = 0;
    this->Cread = 0;

    srcAReader->initialize(count, srcA, xc, [Adata,SRCA_SIZE,count,
        atomicDataPath,this](uint8_t*data, uint8_t size, bool done) 
    {
        assert(size == SRCA_SIZE);
        memcpy(Adata+(SRCA_SIZE*this->Aread), data, SRCA_SIZE);
        ++this->Aread;
        if(done){
            assert(this->Aread == count);
            this->Adone = true;
            if(this->Adone && this->Bdone && this->Cdone){
                atomicDataPath();
            }
        }
    });
    srcBReader->initialize(count, srcB, xc, [Bdata,SRCB_SIZE,count,
        atomicDataPath,this](uint8_t*data, uint8_t size, bool done) 
    {
        assert(size == SRCB_SIZE);
        memcpy(Bdata+(SRCB_SIZE*this->Bread), data, SRCB_SIZE);
        ++this->Bread;
        if(done){
            assert(this->Bread == count);
            this->Bdone = true;
            if(this->Adone && this->Bdone && this->Cdone){
                atomicDataPath();
            }
        }
    });
    srcCReader->initialize(count, srcC, xc, [Cdata,SRCC_SIZE,count,
        atomicDataPath,this](uint8_t*data, uint8_t size, bool done) 
    {
        assert(size == SRCC_SIZE);
        memcpy(Cdata+(SRCC_SIZE*this->Cread), data, SRCC_SIZE);
        ++this->Cread;
        if(done){
            assert(this->Cread == count);
            this->Cdone = true;
            if(this->Adone && this->Bdone && this->Cdone){
                atomicDataPath();
            }
        }
    });
}

//=============================================================================
// LACoreDataOpUnitAtomicParams factory
//=============================================================================

LACoreDataOpUnitAtomic *
LACoreDataOpUnitAtomicParams::create()
{
    return new LACoreDataOpUnitAtomic(this);
}

