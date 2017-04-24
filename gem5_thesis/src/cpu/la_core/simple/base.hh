/*
 * Copyright (c) 2011-2012,2015 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
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
 * Authors: Steve Reinhardt
 *          Dave Greene
 *          Nathan Binkert
 *          Samuel Steffl
 */

#ifndef __CPU_LA_CORE_SIMPLE_BASE_HH__
#define __CPU_LA_CORE_SIMPLE_BASE_HH__

#include <cstdint>
#include <functional>

#include "base/statistics.hh"
#include "config/the_isa.hh"
#include "cpu/base.hh"
#include "cpu/checker/cpu.hh"
#include "cpu/exec_context.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/simple/exec_context.hh"
#include "cpu/la_core/thread.hh"
#include "cpu/pc_event.hh"
#include "cpu/static_inst.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/request.hh"
#include "sim/eventq.hh"
#include "sim/full_system.hh"
#include "sim/system.hh"

// forward declarations
class Checkpoint;
class Process;
class Processor;
class ThreadContext;

namespace TheISA
{
    class DTB;
    class ITB;
}

namespace Trace {
    class InstRecord;
}

struct BaseLACoreSimpleCPUParams;
class BPredUnit;
class LACoreSimpleExecContext;

class BaseLACoreSimpleCPU : public BaseCPU
{
  protected:
    ThreadID curThread;
    BPredUnit *branchPred;

    void checkPcEventQueue();
    void swapActiveThread();

  public:
    BaseLACoreSimpleCPU(BaseLACoreSimpleCPUParams *params);
    virtual ~BaseLACoreSimpleCPU();
    void wakeup(ThreadID tid) override;
    void init() override;
  public:
    Trace::InstRecord *traceData;
    CheckerCPU *checker;

    std::vector<LACoreSimpleExecContext*> threadInfo;
    std::list<ThreadID> activeThreads;

    /** Current instruction */
    TheISA::MachInst inst;
    StaticInstPtr curStaticInst;
    StaticInstPtr curMacroStaticInst;

  protected:
    enum Status {
        Idle,
        Running,
        Faulting,
        ITBWaitResponse,
        IcacheRetry,
        IcacheWaitResponse,
        IcacheWaitSwitch,
        DTBWaitResponse,
        DcacheRetry,
        DcacheWaitResponse,
        DcacheWaitSwitch,
    };

    Status _status;

  public:
    Addr dbg_vtophys(Addr addr);


    void checkForInterrupts();
    void setupFetchRequest(Request *req);
    void preExecute();
    void postExecute();
    void advancePC(const Fault &fault);

    void haltContext(ThreadID thread_num) override;

    // statistics
    void regStats() override;
    void resetStats() override;

    void startup() override;

    virtual Fault readMem(Addr addr, uint8_t* data, unsigned size,
                          Request::Flags flags) = 0;

    virtual Fault initiateMemRead(Addr addr, unsigned size,
                                  Request::Flags flags) = 0;

    virtual Fault writeMem(uint8_t* data, unsigned size, Addr addr,
                           Request::Flags flags, uint64_t* res) = 0;

    void countInst();
    Counter totalInsts() const override;
    Counter totalOps() const override;

    void serializeThread(CheckpointOut &cp, ThreadID tid) const override;
    void unserializeThread(CheckpointIn &cp, ThreadID tid) override;


//=========================================================================
// Additional Memory Interface functions for the LACore
//=========================================================================
    
    //atomic LACore accesses
    virtual Fault writeLAMemAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc) = 0;
    virtual Fault writeLAScratchAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc) = 0;
    virtual Fault readLAMemAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc) = 0;
    virtual Fault readLAScratchAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc) = 0;

    //timing LACore accesses
    virtual bool writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size,
        ThreadContext *tc, uint8_t channel,
        std::function<void(void)> callback) = 0;
    virtual bool writeLAScratchTiming(Addr addr, uint8_t *data, uint8_t size,
        ThreadContext *tc, uint8_t,
        std::function<void(void)> callback) = 0;
    virtual bool readLAMemTiming(Addr addr, uint8_t size, ThreadContext *tc,
        uint8_t channel, std::function<void(uint8_t*,uint8_t)> callback) = 0;
    virtual bool readLAScratchTiming(Addr addr, uint8_t size,
        ThreadContext *tc,
        uint8_t channel, std::function<void(uint8_t*,uint8_t)> callback) = 0;
};

#endif // __CPU_LA_CORE_SIMPLE_BASE_HH__
