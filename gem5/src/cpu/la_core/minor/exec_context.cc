/*
 * Copyright (c) 2014-2015 ARM Limited
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
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * All rights reserved.
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
 * Authors: Kevin Lim
 *          Andreas Sandberg
 *          Mitch Hayenga
 *          Samuel Steffl
 */

#include "cpu/la_core/minor/exec_context.hh"

#include "arch/registers.hh"
#include "base/types.hh"
#include "config/the_isa.hh"
#include "cpu/base.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/minor/cpu.hh"
#include "cpu/la_core/minor/execute.hh"
#include "cpu/la_core/thread.hh"
#include "cpu/static_inst_fwd.hh"
#include "cpu/translation.hh"
#include "mem/request.hh"

namespace Minor {

BaseCPU *
MinorLACoreExecContext::getCpuPtr()
{
    return &cpu;
}


/* MIPS: other thread register reading/writing */
uint64_t
MinorLACoreExecContext::readRegOtherThread(int idx, ThreadID tid)
{
    LACoreThread *other_thread = (tid == InvalidThreadID
        ? &thread : cpu.threads[tid]);

    if (idx < TheISA::FP_Reg_Base) { /* Integer */
        return other_thread->readIntReg(idx);
    } else if (idx < TheISA::Misc_Reg_Base) { /* Float */
        return other_thread->readFloatRegBits(idx
            - TheISA::FP_Reg_Base);
    } else { /* Misc */
        return other_thread->readMiscReg(idx
            - TheISA::Misc_Reg_Base);
    }
}

void
MinorLACoreExecContext::setRegOtherThread(int idx,
    const TheISA::MiscReg &val, ThreadID tid)
{
    LACoreThread *other_thread = (tid == InvalidThreadID
        ? &thread : cpu.threads[tid]);

    if (idx < TheISA::FP_Reg_Base) { /* Integer */
        return other_thread->setIntReg(idx, val);
    } else if (idx < TheISA::Misc_Reg_Base) { /* Float */
        return other_thread->setFloatRegBits(idx
            - TheISA::FP_Reg_Base, val);
    } else { /* Misc */
        return other_thread->setMiscReg(idx
            - TheISA::Misc_Reg_Base, val);
    }
}


Fault 
MinorLACoreExecContext::initiateMemRead(Addr addr, unsigned int size,
    Request::Flags flags)
{
    execute.getLSQ().pushRequest(inst, true /* load */, nullptr,
        size, addr, flags, NULL);
    return NoFault;
}


Fault 
MinorLACoreExecContext::writeMem(uint8_t *data, unsigned int size, Addr addr,
    Request::Flags flags, uint64_t *res)
{
    execute.getLSQ().pushRequest(inst, false /* store */, data,
        size, addr, flags, res);
    return NoFault;
}

void 
MinorLACoreExecContext::setPredicate(bool val)
{
    thread.setPredicate(val);
}


void 
MinorLACoreExecContext::armMonitor(Addr address)
{
    getCpuPtr()->armMonitor(inst->id.threadId, address);
}

bool 
MinorLACoreExecContext::mwait(PacketPtr pkt)
{
    return getCpuPtr()->mwait(inst->id.threadId, pkt); 
}

void 
MinorLACoreExecContext::mwaitAtomic(ThreadContext *tc)
{
    return getCpuPtr()->mwaitAtomic(inst->id.threadId, tc, thread.dtb);
}

AddressMonitor *
MinorLACoreExecContext::getAddrMonitor()
{
    return getCpuPtr()->getCpuAddrMonitor(inst->id.threadId);
}

//=========================================================================
//LACore extension
//=========================================================================

//atomic LACore accesses
Fault 
MinorLACoreExecContext::readLAMemAtomic(Addr addr, uint8_t *data,
    uint64_t size)
{
    return cpu.readLAMemAtomic(addr, data, size, tcBase());
}
Fault 
MinorLACoreExecContext::writeLAMemAtomic(Addr addr, uint8_t *data,
    uint64_t size)
{
    return cpu.writeLAMemAtomic(addr, data, size, tcBase());
}

Fault 
MinorLACoreExecContext::readLAScratchAtomic(Addr addr, uint8_t *data,
    uint64_t size)
{
    return cpu.readLAScratchAtomic(addr, data, size, tcBase());
}

Fault 
MinorLACoreExecContext::writeLAScratchAtomic(Addr addr, uint8_t *data,
    uint64_t size)
{
    return cpu.writeLAScratchAtomic(addr, data, size, tcBase());
}
    

//timing LACore accesses
bool 
MinorLACoreExecContext::writeLAMemTiming(Addr addr, uint8_t *data,
    uint8_t size, uint8_t channel, std::function<void(void)> callback)
{
    return cpu.writeLAMemTiming(addr, data, size, tcBase(), channel,
        callback);
}

bool 
MinorLACoreExecContext::writeLAScratchTiming(Addr addr, uint8_t *data,
    uint8_t size, uint8_t channel, std::function<void(void)> callback)
{
    return cpu.writeLAScratchTiming(addr, data, size, tcBase(), channel,
        callback);
}

bool 
MinorLACoreExecContext::readLAMemTiming(Addr addr, uint8_t size,
    uint8_t channel, std::function<void(uint8_t*,uint8_t)> callback)
{
    return cpu.readLAMemTiming(addr, size, tcBase(), channel, callback);
}

bool 
MinorLACoreExecContext::readLAScratchTiming(Addr addr, uint8_t size,
    uint8_t channel, std::function<void(uint8_t*,uint8_t)> callback)
{
    return cpu.readLAScratchTiming(addr, size, tcBase(), channel, callback);
}

} //Minor
