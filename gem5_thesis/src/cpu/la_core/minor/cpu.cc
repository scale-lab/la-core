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

#include "cpu/la_core/minor/cpu.hh"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>

#include "arch/utility.hh"

#include "cpu/la_core/la_cache/cache.hh"
#include "cpu/la_core/la_cache/cache_tlb.hh"
#include "cpu/la_core/la_cache/translation.hh"
#include "cpu/la_core/la_exec/exec_unit.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/minor/dyn_inst.hh"
#include "cpu/la_core/minor/fetch1.hh"
#include "cpu/la_core/minor/pipeline.hh"
#include "cpu/la_core/packet.hh"
#include "cpu/la_core/req_state.hh"
#include "cpu/la_core/scratchpad/scratchpad.hh"
#include "cpu/la_core/simple/base.hh"
#include "cpu/la_core/simple/exec_context.hh"

#include "debug/Drain.hh"
#include "debug/MinorCPU.hh"
#include "debug/Quiesce.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/MinorLACoreCPU.hh"

MinorLACoreCPU::MinorLACoreCPU(MinorLACoreCPUParams *params) :
    BaseCPU(params),
    threadPolicy(params->threadPolicy),
    //LACore stuff
    laCacheMasterId(params->system->getMasterId(name() + ".la_cache")),
    laCachePort(name() + ".la_cache_port", this, params->mem_unit_channels),
    laCache(params->laCache), laCacheTLB(params->laCacheTLB),
    scratchpad(params->scratchpad),
    uniqueReqId(0), laExecUnit(params->laExecUnit)
{
    /* This is only written for one thread at the moment */
    Minor::MinorThread *thread;

    //create an independent port for each mem_unit to the scratchpad
    for (uint8_t i=0; i<params->mem_unit_channels; ++i) {
        scratchMasterIds.push_back(
            params->system->getMasterId(name() +
                ".scratch" + std::to_string(i))),
        scratchPorts.push_back(
            ScratchPort(name() + ".scratch_port", this, i));
    }

    for (ThreadID i = 0; i < numThreads; i++) {
        if (FullSystem) {
            thread = new Minor::MinorThread(this, i, params->system,
                    params->itb, params->dtb, params->isa[i]);
            thread->setStatus(ThreadContext::Halted);
        } else {
            thread = new Minor::MinorThread(this, i, params->system,
                    params->workload[i], params->itb, params->dtb,
                    params->isa[i]);
        }

        threads.push_back(thread);
        ThreadContext *tc = thread->getTC();
        threadContexts.push_back(tc);
    }


    if (params->checker) {
        fatal("The Minor model doesn't support checking (yet)\n");
    }

    Minor::MinorDynInst::init();

    pipeline = new Minor::Pipeline(*this, *params);
    activityRecorder = pipeline->getActivityRecorder();
}

MinorLACoreCPU::~MinorLACoreCPU()
{
    delete pipeline;

    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        delete threads[thread_id];
    }
}

void
MinorLACoreCPU::init()
{
    BaseCPU::init();

    if (!params()->switched_out &&
        system->getMemoryMode() != Enums::timing)
    {
        fatal("The Minor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    /* Initialise the ThreadContext's memory proxies */
    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        ThreadContext *tc = getContext(thread_id);

        tc->initMemProxies(tc);
    }

    /* Initialise CPUs (== threads in the ISA) */
    if (FullSystem && !params()->switched_out) {
        for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++)
        {
            ThreadContext *tc = getContext(thread_id);

            /* Initialize CPU, including PC */
            TheISA::initCPU(tc, cpuId());
        }
    }
}

/** Stats interface from SimObject (by way of BaseCPU) */
void
MinorLACoreCPU::regStats()
{
    BaseCPU::regStats();
    stats.regStats(name(), *this);
    pipeline->regStats();
}

void
MinorLACoreCPU::serializeThread(CheckpointOut &cp, ThreadID thread_id) const
{
    threads[thread_id]->serialize(cp);
}

void
MinorLACoreCPU::unserializeThread(CheckpointIn &cp, ThreadID thread_id)
{
    threads[thread_id]->unserialize(cp);
}

void
MinorLACoreCPU::serialize(CheckpointOut &cp) const
{
    pipeline->serialize(cp);
    BaseCPU::serialize(cp);
}

void
MinorLACoreCPU::unserialize(CheckpointIn &cp)
{
    pipeline->unserialize(cp);
    BaseCPU::unserialize(cp);
}

Addr
MinorLACoreCPU::dbg_vtophys(Addr addr)
{
    /* Note that this gives you the translation for thread 0 */
    panic("No implementation for vtophy\n");

    return 0;
}

void
MinorLACoreCPU::wakeup(ThreadID tid)
{
    DPRINTF(Drain, "[tid:%d] MinorCPU wakeup\n", tid);
    assert(tid < numThreads);

    if (threads[tid]->status() == ThreadContext::Suspended) {
        threads[tid]->activate();
    }
}

void
MinorLACoreCPU::startup()
{
    DPRINTF(MinorCPU, "MinorCPU startup\n");

    BaseCPU::startup();

    for (auto i = threads.begin(); i != threads.end(); i ++)
        (*i)->startup();

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        threads[tid]->startup();
        pipeline->wakeupFetch(tid);
    }
}

DrainState
MinorLACoreCPU::drain()
{
    if (switchedOut()) {
        DPRINTF(Drain, "Minor CPU switched out, draining not needed.\n");
        return DrainState::Drained;
    }

    DPRINTF(Drain, "MinorCPU drain\n");

    /* Need to suspend all threads and wait for Execute to idle.
     * Tell Fetch1 not to fetch */
    if (pipeline->drain()) {
        DPRINTF(Drain, "MinorCPU drained\n");
        return DrainState::Drained;
    } else {
        DPRINTF(Drain, "MinorCPU not finished draining\n");
        return DrainState::Draining;
    }
}

void
MinorLACoreCPU::signalDrainDone()
{
    DPRINTF(Drain, "MinorCPU drain done\n");
    Drainable::signalDrainDone();
}

void
MinorLACoreCPU::drainResume()
{
    /* When taking over from another cpu make sure lastStopped
     * is reset since it might have not been defined previously
     * and might lead to a stats corruption */
    pipeline->resetLastStopped();

    if (switchedOut()) {
        DPRINTF(Drain, "drainResume while switched out.  Ignoring\n");
        return;
    }

    DPRINTF(Drain, "MinorCPU drainResume\n");

    if (!system->isTimingMode()) {
        fatal("The Minor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    for (ThreadID tid = 0; tid < numThreads; tid++)
        wakeup(tid);

    pipeline->drainResume();
}

void
MinorLACoreCPU::memWriteback()
{
    DPRINTF(Drain, "MinorCPU memWriteback\n");
}

void
MinorLACoreCPU::switchOut()
{
    DPRINTF(MinorCPU, "MinorCPU switchOut\n");

    assert(!switchedOut());
    BaseCPU::switchOut();

    /* Check that the CPU is drained? */
    activityRecorder->reset();
}

void
MinorLACoreCPU::takeOverFrom(BaseCPU *old_cpu)
{
    DPRINTF(MinorCPU, "MinorCPU takeOverFrom\n");

    BaseCPU::takeOverFrom(old_cpu);
}

void
MinorLACoreCPU::activateContext(ThreadID thread_id)
{
    DPRINTF(MinorCPU, "ActivateContext thread: %d\n", thread_id);

    /* Do some cycle accounting.  lastStopped is reset to stop the
     *  wakeup call on the pipeline from adding the quiesce period
     *  to BaseCPU::numCycles */
    stats.quiesceCycles += pipeline->cyclesSinceLastStopped();
    pipeline->resetLastStopped();

    /* Wake up the thread, wakeup the pipeline tick */
    threads[thread_id]->activate();
    wakeupOnEvent(Minor::Pipeline::CPUStageId);
    pipeline->wakeupFetch(thread_id);

    BaseCPU::activateContext(thread_id);
}

void
MinorLACoreCPU::suspendContext(ThreadID thread_id)
{
    DPRINTF(MinorCPU, "SuspendContext %d\n", thread_id);

    threads[thread_id]->suspend();

    BaseCPU::suspendContext(thread_id);
}

void
MinorLACoreCPU::wakeupOnEvent(unsigned int stage_id)
{
    DPRINTF(Quiesce, "Event wakeup from stage %d\n", stage_id);

    /* Mark that some activity has taken place and start the pipeline */
    activityRecorder->activateStage(stage_id);
    pipeline->start();
}

MinorLACoreCPU *
MinorLACoreCPUParams::create()
{
    return new MinorLACoreCPU(this);
}

MasterPort &MinorLACoreCPU::getInstPort()
{
    return pipeline->getInstPort();
}

MasterPort &MinorLACoreCPU::getDataPort()
{
    return pipeline->getDataPort();
}

Counter
MinorLACoreCPU::totalInsts() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numInst;

    return ret;
}

Counter
MinorLACoreCPU::totalOps() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numOp;

    return ret;
}












//=============================================================================
// Port Functions
//=============================================================================

BaseMasterPort &
MinorLACoreCPU::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "scratch_port")
        return scratchPorts[idx];
    else if (if_name == "la_cache_port")
        return getLACachePort();
    else
        return BaseCPU::getMasterPort(if_name, idx);
}

//=============================================================================
// LACachePort functions
//=============================================================================

MinorLACoreCPU::LACachePort::LACachePort(
    const std::string& name, MinorLACoreCPU* cpu, uint8_t channels) :
    MasterPort(name, cpu), cpu(cpu)
{
    //create the queues for each of the channels to the LACache
    for (uint8_t i=0; i<channels; ++i) {
        laCachePktQs.push_back(std::deque<PacketPtr>());
    }
}

MinorLACoreCPU::LACachePort::~LACachePort()
{
}

// submits a request for translation to the execCacheTlb
// MEMCPY data if present, so caller must deallocate it himself!
bool
MinorLACoreCPU::LACachePort::startTranslation(Addr addr, uint8_t *data,
    uint64_t size, BaseTLB::Mode mode, ThreadContext *tc, uint64_t req_id,
    uint8_t channel)
{
    //TODO: just for now, make sure all data is page-aligned, no split-reqs
    //      This shouldn't be a problem on Linux - i think everything % 8 == 0
    Process * p = tc->getProcessPtr();
    Addr page1 = p->pTable->pageAlign(addr);
    Addr page2 = p->pTable->pageAlign(addr+size-1);
    assert(page1 == page2);

    //NOTE: need to make a buffer for reads so cache can write to it!
    uint8_t *ndata = new uint8_t[size];
    if (data != nullptr) {
        assert(mode == BaseTLB::Write);
        memcpy(ndata, data, size);
    } else {
      //put a pattern here for debugging
      memset(ndata, 'Z', size);
    }
    MemCmd cmd = (mode==BaseTLB::Write) ? MemCmd::WriteReq :
        MemCmd::ReadReq;

    //virtual address request constructor (copied the data port request)
    const int asid = 0;
    const Addr pc = tc->instAddr();
    RequestPtr req = new Request(asid, addr, size, 0, cpu->laCacheMasterId, pc,
        tc->contextId());
    req->taskId(cpu->taskId());

    //start translation (translation gets auto-deleted after the event
    // gets processed!)
    LACoreLACacheTranslation *translation = new LACoreLACacheTranslation(cpu,
        [cmd,req,req_id,channel,ndata,this](Fault fault)
    {
        assert(fault == NoFault);
        PacketPtr pkt = new LACorePacket(req, cmd, req_id, channel);
        pkt->dataDynamic(ndata);

        if (!sendTimingReq(pkt)) {
            laCachePktQs[channel].push_back(pkt);
        }
    });
    cpu->laCacheTLB->translateTiming(req, tc, translation);
    //HACK: STEFFL: doing atomic translations right now, typically do NOT
    //  call delete here!!
    delete translation;

    return true;
}

bool
MinorLACoreCPU::LACachePort::sendTimingReadReq(Addr addr, uint64_t size,
    ThreadContext *tc, uint64_t req_id, uint8_t channel)
{
    return startTranslation(addr, nullptr, size, BaseTLB::Read, tc, req_id,
        channel);
}

bool
MinorLACoreCPU::LACachePort::sendTimingWriteReq(Addr addr,
    uint8_t *data, uint64_t size, ThreadContext *tc, uint64_t req_id,
    uint8_t channel)
{
    return startTranslation(addr, data, size, BaseTLB::Write, tc, req_id,
        channel);
}

bool
MinorLACoreCPU::LACachePort::recvTimingResp(PacketPtr pkt)
{
    LACorePacketPtr la_pkt = dynamic_cast<LACorePacketPtr>(pkt);
    assert(la_pkt != nullptr);
    cpu->recvTimingResp(la_pkt);
    return true;
}

void
MinorLACoreCPU::LACachePort::recvReqRetry()
{
    //TODO: must be a better way to figure out which channel specified the
    //      retry
    for(auto& laCachePktQ : laCachePktQs) {
        //assert(laCachePktQ.size());
        while (laCachePktQ.size() && sendTimingReq(laCachePktQ.front())) {
            // memory system takes ownership of packet
            laCachePktQ.pop_front();
        }
    }
}

//=============================================================================
// ScratchpadPort functions
//=============================================================================

MinorLACoreCPU::ScratchPort::ScratchPort(const std::string& name,
    MinorLACoreCPU* cpu, uint64_t channel) :
    MasterPort(name, cpu), cpu(cpu), channel(channel)
{
}

MinorLACoreCPU::ScratchPort::~ScratchPort()
{
}

bool
MinorLACoreCPU::ScratchPort::sendTimingReadReq(Addr addr, uint64_t size,
    ThreadContext *tc, uint64_t req_id)
{
    //physical addressing
    RequestPtr req = new Request(addr, size, 0,
        cpu->scratchMasterIds[channel]);
    PacketPtr pkt = new LACorePacket(req, MemCmd::ReadReq, req_id);

    //make data for cache to put data into
    uint8_t *ndata = new uint8_t[size];
    memset(ndata, 'Z', size);
    pkt->dataDynamic(ndata);

    if (!sendTimingReq(pkt)) {
        delete pkt->req;
        delete pkt;         //delete pkt deletes ndata!
        return false;
    }
    return true;
}

// memcpy of data, so the caller can delete it when it pleases
bool
MinorLACoreCPU::ScratchPort::sendTimingWriteReq(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc, uint64_t req_id)
{

    //physical addressing
    RequestPtr req = new Request(addr, size, 0,
        cpu->scratchMasterIds[channel]);
    LACorePacketPtr pkt = new LACorePacket(req, MemCmd::WriteReq, req_id);

    //make copy of data here
    uint8_t *ndata = new uint8_t[size];
    memcpy(ndata, data, size);
    pkt->dataDynamic(ndata);

    if(!sendTimingReq(pkt)){
        delete pkt->req;
        delete pkt;         //NOTE: delete pkt will delete ndata
        return false;
    }
    return true;
}

bool
MinorLACoreCPU::ScratchPort::recvTimingResp(PacketPtr pkt)
{
    LACorePacketPtr la_pkt = dynamic_cast<LACorePacketPtr>(pkt);
    assert(la_pkt != nullptr);
    cpu->recvTimingResp(la_pkt);
    return true;
}

void
MinorLACoreCPU::ScratchPort::recvReqRetry()
{
    //do nothing, caller will manually resend request
}
   
//=========================================================================
//LACore handle timing responses from scratchpad and memory
//========================================================================= 

void
MinorLACoreCPU::recvTimingResp(LACorePacketPtr la_pkt)
{
    //NOTE: i'm not sure this is relevent for MinorCPU? but for Simple yes
    //update cycle count
    //updateCycleCounts();

    //find the associated request in the queue and associate with la_pkt
    assert(laPendingReqQ.size());
    bool found = false;
    for(LAMemReqState * pending : laPendingReqQ) {
        if(pending->getReqId() == la_pkt->reqId){
            pending->setPacket(la_pkt);
            found = true;
            break;
        }
    }
    assert(found);

    //commit each request in order they were issued
    while (laPendingReqQ.size() && laPendingReqQ.front()->isMatched()) {
        LAMemReqState * pending = laPendingReqQ.front();
        laPendingReqQ.pop_front();
        pending->executeCallback();

        //NOTE: pending deletes packet. packet calls delete[] on the data
        delete pending;
    }
}

//=========================================================================
//LACore ATOMIC Memory and Scratchpad interface
//=========================================================================

Fault 
MinorLACoreCPU::writeLAMemAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    panic("MinorLACoreCPU::writeLAMemAtomic should not be called");
}

Fault 
MinorLACoreCPU::writeLAScratchAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    panic("MinorLACoreCPU::writeLAScratchAtomic should not be called");
}

Fault 
MinorLACoreCPU::readLAMemAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    panic("MinorLACoreCPU::readLAMemAtomic should not be called");
}

Fault 
MinorLACoreCPU::readLAScratchAtomic(Addr addr, uint8_t *data,
    uint64_t size, ThreadContext *tc)
{
    panic("MinorLACoreCPU::readLAScratchAtomic should not be called");
}

//=========================================================================
//LACore TIMING Memory and Scratchpad interface
//=========================================================================

//
// NOTE: does NOT memcpy data, the ports do that!
//
bool
MinorLACoreCPU::writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size,
    ThreadContext *tc, uint8_t channel, std::function<void(void)> callback)
{
    uint64_t id = (uniqueReqId++);
    LAMemReqState *pending = new LAMemWriteReqState(id, callback);
    laPendingReqQ.push_back(pending);
    if (!laCachePort.sendTimingWriteReq(addr, data, size, tc, id, channel)) {
        delete laPendingReqQ.back();
        laPendingReqQ.pop_back();
        return false;
    }
    return true;
}

//
// NOTE: does NOT memcpy data, the ports do that!
//
bool
MinorLACoreCPU::writeLAScratchTiming(Addr addr, uint8_t *data,
    uint8_t size, ThreadContext *tc, uint8_t channel,
    std::function<void(void)> callback)
{
    uint64_t id = (uniqueReqId++);
    LAMemReqState *pending = new LAMemWriteReqState(id, callback);
    laPendingReqQ.push_back(pending);
    if (!scratchPorts[channel].sendTimingWriteReq(addr, data, size, tc, id)) {
        delete laPendingReqQ.back();
        laPendingReqQ.pop_back();
        return false;
    }
    return true;
}

//
//TODO: returns true on success. although right now, it will never fail!
//
bool
MinorLACoreCPU::readLAMemTiming(Addr addr, uint8_t size, 
    ThreadContext *tc, uint8_t channel,
    std::function<void(uint8_t*,uint8_t)> callback)
{
    uint64_t id = (uniqueReqId++);
    LAMemReqState *pending = new LAMemReadReqState(id, callback);
    laPendingReqQ.push_back(pending);
    if (!laCachePort.sendTimingReadReq(addr, size, tc, id, channel)) {
        delete laPendingReqQ.back();
        laPendingReqQ.pop_back();
        return false;
    }
    return true;
}

bool
MinorLACoreCPU::readLAScratchTiming(Addr addr, uint8_t size,
    ThreadContext *tc, uint8_t channel,
    std::function<void(uint8_t*,uint8_t)> callback)
{
    uint64_t id = (uniqueReqId++);
    LAMemReqState *pending = new LAMemReadReqState(id, callback);
    laPendingReqQ.push_back(pending);
    if (!scratchPorts[channel].sendTimingReadReq(addr, size, tc, id)) {
        delete laPendingReqQ.back();
        laPendingReqQ.pop_back();
        return false;
    }
    return true;
}
