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

#ifndef __CPU_LA_CORE_EXEC_CONTEXT_HH__
#define __CPU_LA_CORE_EXEC_CONTEXT_HH__

#include <cstdint>
#include <functional>

#include "base/types.hh"
#include "cpu/exec_context.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/thread.hh"
#include "mem/request.hh"

class LACoreExecContext : public ExecContext {

  public:
    LACoreThread* thread;

    LACoreExecContext(LACoreThread* _thread) :
        thread(_thread) {}
    ~LACoreExecContext() {}

//=========================================================================
// LACore Register Accesses
//=========================================================================

    //convenience functions -- don't want to deal with operand positioning
    virtual uint64_t readIntRegFlat(int idx) = 0;
    virtual void setIntRegFlat(int idx, uint64_t val) = 0;
    virtual FloatRegBits readFloatRegBitsFlat(int idx) = 0;
    virtual void setFloatRegBitsFlat(int idx, FloatRegBits val) = 0;

    //implementation of the following depends on specific CPU model
    virtual LAReg& getLAReg(unsigned idx) = 0;
    virtual LACsrReg& getLACsrReg() = 0;

//=========================================================================
// LACore Reading/Writing to Scratchpad/memory
//=========================================================================

    //atomic LACore accesses
    virtual Fault readLAMemAtomic(Addr addr, uint8_t *data, uint64_t size) = 0;
    virtual Fault writeLAMemAtomic(Addr addr, uint8_t *data, uint64_t size) = 0;
    virtual Fault readLAScratchAtomic(Addr addr, uint8_t *data, 
        uint64_t size) = 0;
    virtual Fault writeLAScratchAtomic(Addr addr, uint8_t *data, 
        uint64_t size) = 0;
    
    //timing LACore accesses
    virtual bool writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size, 
        uint8_t channel, std::function<void(void)> callback) = 0;
    virtual bool writeLAScratchTiming(Addr addr, uint8_t *data, uint8_t size, 
        uint8_t channel, std::function<void(void)> callback) = 0;
    virtual bool readLAMemTiming(Addr addr, uint8_t size, 
        uint8_t channel, std::function<void(uint8_t*,uint8_t)> callback) = 0;
    virtual bool readLAScratchTiming(Addr addr, uint8_t size, 
        uint8_t channel, std::function<void(uint8_t*,uint8_t)> callback) = 0;

};

#endif // __CPU_LA_CORE_EXEC_CONTEXT_HH__
