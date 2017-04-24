/*
 * Copyright (c) 2012-2014 ARM Limited
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
 * Authors: Andrew Bardsley
 */

/**
 * @file
 *
 *  Top level definition of the Minor in-order CPU model
 */

#ifndef __CPU_LA_CORE_MINOR_CPU_HH__
#define __CPU_LA_CORE_MINOR_CPU_HH__

#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

#include "cpu/base.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_cache/cache.hh"
#include "cpu/la_core/la_cache/cache_tlb.hh"
#include "cpu/la_core/la_cache/translation.hh"
#include "cpu/la_core/la_exec/exec_unit.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/minor/activity.hh"
#include "cpu/la_core/minor/exec_context.hh"
#include "cpu/la_core/minor/stats.hh"
#include "cpu/la_core/packet.hh"
#include "cpu/la_core/req_state.hh"
#include "cpu/la_core/scratchpad/scratchpad.hh"
#include "cpu/la_core/thread.hh"
#include "cpu/translation.hh"
#include "enums/ThreadPolicy.hh"
#include "mem/request.hh"
#include "params/MinorLACoreCPU.hh"

namespace Minor
{
/** Forward declared to break the cyclic inclusion dependencies between
 *  pipeline and cpu */
class Pipeline;

/** Minor will use the SimpleThread state for now */
typedef LACoreThread MinorThread;
};

/**
 *  MinorCPU is an in-order CPU model with four fixed pipeline stages:
 *
 *  Fetch1 - fetches lines from memory
 *  Fetch2 - decomposes lines into macro-op instructions
 *  Decode - decomposes macro-ops into micro-ops
 *  Execute - executes those micro-ops
 *
 *  This pipeline is carried in the MinorCPU::pipeline object.
 *  The exec_context interface is not carried by MinorCPU but by
 *      Minor::ExecContext objects
 *  created by Minor::Execute.
 */
class MinorLACoreCPU : public BaseCPU
{
  protected:
    /** pipeline is a container for the clockable pipeline stage objects.
     *  Elements of pipeline call TheISA to implement the model. */
    Minor::Pipeline *pipeline;

  public:
    /** Activity recording for pipeline.  This belongs to Pipeline but
     *  stages will access it through the CPU as the MinorCPU object
     *  actually mediates idling behaviour */
    Minor::MinorActivityRecorder *activityRecorder;

    /** These are thread state-representing objects for this CPU.  If
     *  you need a ThreadContext for *any* reason, use
     *  threads[threadId]->getTC() */
    std::vector<Minor::MinorThread *> threads;

  public:
    /** Provide a non-protected base class for Minor's Ports as derived
     *  classes are created by Fetch1 and Execute */
    class MinorCPUPort : public MasterPort
    {
      public:
        /** The enclosing cpu */
        MinorLACoreCPU &cpu;

      public:
        MinorCPUPort(const std::string& name_, MinorLACoreCPU &cpu_)
            : MasterPort(name_, &cpu_), cpu(cpu_)
        { }

    };

    /** Thread Scheduling Policy (RoundRobin, Random, etc) */
    Enums::ThreadPolicy threadPolicy;
  protected:
     /** Return a reference to the data port. */
    MasterPort &getDataPort() override;

    /** Return a reference to the instruction port. */
    MasterPort &getInstPort() override;

  public:
    MinorLACoreCPU(MinorLACoreCPUParams *params);

    ~MinorLACoreCPU();

  public:
    /** Starting, waking and initialisation */
    void init() override;
    void startup() override;
    void wakeup(ThreadID tid) override;

    Addr dbg_vtophys(Addr addr);

    /** Processor-specific statistics */
    Minor::MinorStats stats;

    /** Stats interface from SimObject (by way of BaseCPU) */
    void regStats() override;

    /** Simple inst count interface from BaseCPU */
    Counter totalInsts() const override;
    Counter totalOps() const override;

    void serializeThread(CheckpointOut &cp, ThreadID tid) const override;
    void unserializeThread(CheckpointIn &cp, ThreadID tid) override;

    /** Serialize pipeline data */
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    /** Drain interface */
    DrainState drain() override;
    void drainResume() override;
    /** Signal from Pipeline that MinorCPU should signal that a drain
     *  is complete and set its drainState */
    void signalDrainDone();
    void memWriteback() override;

    /** Switching interface from BaseCPU */
    void switchOut() override;
    void takeOverFrom(BaseCPU *old_cpu) override;

    /** Thread activation interface from BaseCPU. */
    void activateContext(ThreadID thread_id) override;
    void suspendContext(ThreadID thread_id) override;

    /** Thread scheduling utility functions */
    std::vector<ThreadID> roundRobinPriority(ThreadID priority)
    {
        std::vector<ThreadID> prio_list;
        for (ThreadID i = 1; i <= numThreads; i++) {
            prio_list.push_back((priority + i) % numThreads);
        }
        return prio_list;
    }

    std::vector<ThreadID> randomPriority()
    {
        std::vector<ThreadID> prio_list;
        for (ThreadID i = 0; i < numThreads; i++) {
            prio_list.push_back(i);
        }
        std::random_shuffle(prio_list.begin(), prio_list.end());
        return prio_list;
    }

    /** Interface for stages to signal that they have become active after
     *  a callback or eventq event where the pipeline itself may have
     *  already been idled.  The stage argument should be from the
     *  enumeration Pipeline::StageId */
    void wakeupOnEvent(unsigned int stage_id);

//=========================================================================
// Extra port and functions needed for LACore extension
//=========================================================================

  public:
    class LACachePort : public MasterPort
    {
      public:
        LACachePort(const std::string& name, MinorLACoreCPU* cpu,
            uint8_t channels);
        ~LACachePort();

        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;

        bool startTranslation(Addr addr, uint8_t *data, uint64_t size, 
            BaseTLB::Mode mode, ThreadContext *tc, uint64_t req_id,
            uint8_t channel);
        bool sendTimingReadReq(Addr addr, uint64_t size, ThreadContext *tc,
            uint64_t req_id, uint8_t channel);
        bool sendTimingWriteReq(Addr addr, uint8_t *data, uint64_t size,
            ThreadContext *tc, uint64_t req_id, uint8_t channel);

        std::vector< std::deque<PacketPtr> > laCachePktQs;
        MinorLACoreCPU *cpu;
    };

    class ScratchPort : public MasterPort
    {
      public:
        ScratchPort(const std::string& name, MinorLACoreCPU* cpu,
            uint64_t channel);
        ~ScratchPort();

        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;

        bool sendTimingReadReq(Addr addr, uint64_t size, ThreadContext *tc, 
            uint64_t req_id);
        bool sendTimingWriteReq(Addr addr, uint8_t *data, uint64_t size,
            ThreadContext *tc, uint64_t req_id);

        MinorLACoreCPU *cpu;
        const uint64_t channel;
    };

    //used to identify ports uniquely to whole memory system
    MasterID laCacheMasterId;
    std::vector<MasterID> scratchMasterIds;

    LACachePort laCachePort;
    std::vector<ScratchPort> scratchPorts;

    MasterPort &getLACachePort() { return laCachePort; }

    BaseMasterPort &getMasterPort(const std::string &if_name,
                                  PortID idx = InvalidPortID) override;
    
    LACoreLACache * laCache;
    LACoreLACacheTLB * laCacheTLB;
    LACoreScratchpad * scratchpad;

//=========================================================================
// Read/Write to Mem/Scratch interface functions!
//=========================================================================

    //called by memory/scratchpad ports on data received during load
    void recvTimingResp(LACorePacketPtr pkt);

    //atomic LACore accesses
    Fault writeLAMemAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc);
    Fault writeLAScratchAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc);
    Fault readLAMemAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc);
    Fault readLAScratchAtomic(Addr addr, uint8_t *data, uint64_t size,
        ThreadContext *tc);

    //timing LACore accesses
    bool writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size,
        ThreadContext *tc, uint8_t channel,
        std::function<void(void)> callback);
    bool writeLAScratchTiming(Addr addr, uint8_t *data, uint8_t size,
        ThreadContext *tc, uint8_t channel,
        std::function<void(void)> callback);
    bool readLAMemTiming(Addr addr, uint8_t size, ThreadContext *tc,
        uint8_t channel,
        std::function<void(uint8_t*,uint8_t)> callback);
    bool readLAScratchTiming(Addr addr, uint8_t size, ThreadContext *tc,
        uint8_t channel,
        std::function<void(uint8_t*,uint8_t)> callback);

    //fields for keeping track of outstanding requests for reordering
    uint64_t uniqueReqId;
    std::deque<LAMemReqState *> laPendingReqQ;

    //the actual execution unit!
    LACoreExecUnit *laExecUnit;

};

#endif /* __CPU_LA_CORE_MINOR_CPU_HH__ */
