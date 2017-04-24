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

#include "cpu/la_core/la_exec/cfg_op_unit.hh"

#include <functional>

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_insn.hh"
#include "params/LACoreCfgOpUnit.hh"
#include "sim/faults.hh"
#include "sim/sim_object.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreCfgOpUnit::LACoreCfgOpUnit(const LACoreCfgOpUnitParams *p) :
    SimObject(p)
{
}

LACoreCfgOpUnit::~LACoreCfgOpUnit()
{
}

//=============================================================================
// interface functions
//=============================================================================

bool LACoreCfgOpUnit::isOccupied()
{
    //assuming single threaded simulator, so this will never get called when
    //this is handling an issued instruction
    return false;
}

void LACoreCfgOpUnit::issue(LAInsn& insn, LACoreExecContext *xc,
    std::function<void(Fault fault)> done_callback)
{
    LAReg * dst = &(xc->getLAReg(insn.lrd()));

    //set the flags. not all flags are updated on every cfg!
    bool has_dp         = (!insn.vec() || (insn.vec() && insn.alt()));
    bool has_vector     = true;
    bool has_sparse     = (insn.vec() == 1 && insn.alt() == 0);
    bool has_transpose  = (has_sparse && insn.spv());

    dst->dp(       has_dp         ? insn.dp()  : dst->dp());
    dst->vector(   has_vector     ? insn.vec() : dst->vector());
    dst->sparse(   has_sparse     ? insn.spv() : dst->sparse());
    dst->transpose(has_transpose  ? insn.tns() : dst->transpose());

    //set the scalar values if applicable
    if(!insn.vec()){
        dst->loc((LALoc)insn.lrdloc());
        if(!insn.alt()){
            dst->scalarAddr(xc->readIntRegFlat(insn.lrxa()));
        } else if(insn.dp()){
            uint64_t tmp_dp = xc->readFloatRegBitsFlat(insn.lrfa());
            dst->scalarDouble(*(double *)&tmp_dp);
        } else {
            uint32_t tmp_fp = xc->readFloatRegBitsFlat(insn.lrfa());
            dst->scalarFloat(*(float *)&tmp_fp);
        }
    }

    //set the vector addr field if applicable
    if(insn.vec() && !insn.alt() && !insn.spv()){
        dst->vecAddr((LAAddr)xc->readIntRegFlat(insn.lrxa()));
    }

    //set the sparse addr fields if applicable
    if(insn.vec() && !insn.alt() && insn.spv()){
        dst->dataPtr((LAAddr)xc->readIntRegFlat(insn.lrxa()));
        dst->idxPtr((LAAddr)xc->readIntRegFlat(insn.lrxb()));
        dst->jdxPtr((LAAddr)xc->readIntRegFlat(insn.lrxc()));
    }

    //set the layout if applicable (sparse and vector redundant here, so only
    //set the vecotr one
    if(insn.vec() && insn.alt()){
        dst->loc(insn.lrdloc());
        dst->vecStride((LAVecStride)xc->readIntRegFlat(insn.lrxa()));
        dst->vecCount((LAVecCount)xc->readIntRegFlat(insn.lrxb()));
        dst->vecSkip((LAVecSkip)xc->readIntRegFlat(insn.lrxc()));
    }

    //finishes in 1 cycle
    done_callback(NoFault);
}

//=============================================================================
// LACoreCfgOpUnitParams factory
//=============================================================================

LACoreCfgOpUnit *
LACoreCfgOpUnitParams::create()
{
    return new LACoreCfgOpUnit(this);
}

