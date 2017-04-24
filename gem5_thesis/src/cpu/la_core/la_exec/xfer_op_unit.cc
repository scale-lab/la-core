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

#include "cpu/la_core/la_exec/xfer_op_unit.hh"

#include <cstdint>
#include <functional>

#include "base/misc.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_mem_unit/read_timing_unit.hh"
#include "cpu/la_core/la_mem_unit/write_timing_unit.hh"
#include "params/LACoreXferOpUnit.hh"
#include "sim/faults.hh"
#include "sim/sim_object.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreXferOpUnit::LACoreXferOpUnit(const LACoreXferOpUnitParams *p) :
    SimObject(p), 
    occupied(false), memReader(p->memReader), memWriter(p->memWriter)
{
}

LACoreXferOpUnit::~LACoreXferOpUnit()
{
}

//=============================================================================
// interface functions
//=============================================================================

bool LACoreXferOpUnit::isOccupied()
{
    return occupied;
}

void LACoreXferOpUnit::issue(LAInsn& insn, LACoreExecContext *xc,
    std::function<void(Fault fault)> done_callback)
{
    assert(!occupied);
    occupied = true;

    LAReg * dst = &(xc->getLAReg(insn.lrd()));
    LAReg * src = &(xc->getLAReg(insn.lra()));

    if (insn.dat()) {
        LACount count = xc->readIntRegFlat(insn.lrxa());
        uint8_t DST_SIZE = dst->dp() ? sizeof(double) : sizeof(float);

        //no-ops are fine
        if (count == 0) {
            occupied = false;
            done_callback(NoFault);
            return;
        }

        //NOTE: need to initialize the writer BEFORE the reader!
        memWriter->initialize(count, dst, xc, [done_callback,this](bool done){
            if (done) {
                this->occupied = false;
                done_callback(NoFault);
            }
        });
        memReader->initialize(count, src, xc, 
            [src,dst,DST_SIZE,this](uint8_t*data, uint8_t size, bool done)
        {
            if(src->dp() && !dst->dp()){
                uint8_t *ndata = new uint8_t[DST_SIZE];
                *((float *)ndata) = (float)(*(double *)data);
                this->memWriter->queueData(ndata);
            } else if(!src->dp() && dst->dp()) {
                uint8_t *ndata = new uint8_t[DST_SIZE];
                *((double *)ndata) = (double)(*(float *)data);
                this->memWriter->queueData(ndata);
            } else {
                uint8_t *ndata = new uint8_t[DST_SIZE];
                memcpy(ndata, data, DST_SIZE);
                this->memWriter->queueData(ndata);
            }
            //see contract for memReader -- we must delete data
            delete[] data;
        });
    } else if(insn.get()) {
        xc->setIntRegFlat(insn.lrxa(), xc->getLACsrReg().readAll());
        occupied = false;
        done_callback(NoFault);
    } else if(insn.clr()) {
        xc->getLACsrReg().readAll(); 
        occupied = false;
        done_callback(NoFault);
    } else {
        panic("invalid LACoreXfer insn fields");
    }
}

//=============================================================================
// LACoreXferOpUnitParams factory
//=============================================================================

LACoreXferOpUnit *
LACoreXferOpUnitParams::create()
{
    return new LACoreXferOpUnit(this);
}

