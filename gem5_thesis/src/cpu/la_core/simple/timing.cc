/*
 * Copyright 2014 Google, Inc.
 * Copyright (c) 2010-2013,2015 ARM Limited
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
 *          Samuel Steffl
 */

#include "cpu/la_core/simple/timing.hh"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>

#include "arch/locked_mem.hh"
#include "arch/mmapped_ipr.hh"
#include "arch/utility.hh"
#include "base/bigint.hh"
#include "base/misc.hh"
#include "config/the_isa.hh"
#include "cpu/exetrace.hh"

#include "cpu/la_core/la_cache/cache.hh"
#include "cpu/la_core/la_cache/cache_tlb.hh"
#include "cpu/la_core/la_cache/translation.hh"
#include "cpu/la_core/la_exec/exec_unit.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/packet.hh"
#include "cpu/la_core/req_state.hh"
#include "cpu/la_core/scratchpad/scratchpad.hh"
#include "cpu/la_core/simple/base.hh"
#include "cpu/la_core/simple/exec_context.hh"

#include "debug/Config.hh"
#include "debug/Drain.hh"
#include "debug/ExecFaulting.hh"
#include "debug/LACoreSimpleCPU.hh"
#include "debug/Mwait.hh"
#include "debug/TimingLACoreSimpleCPU.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/TimingLACoreSimpleCPU.hh"
#include "sim/faults.hh"
#include "sim/full_system.hh"
#include "sim/system.hh"

using namespace std;
using namespace TheISA;

void
TimingLACoreSimpleCPU::init()
{
    BaseLACoreSimpleCPU::init();
}

void
TimingLACoreSimpleCPU::TimingCPUPort::TickEvent::schedule(PacketPtr _pkt, 
    Tick t)
{
    pkt = _pkt;
    cpu->schedule(this, t);
}

TimingLACoreSimpleCPU::TimingLACoreSimpleCPU(TimingLACoreSimpleCPUParams *p) : 
    BaseLACoreSimpleCPU(p), fetchTranslation(this), icachePort(this),
    dcachePort(this), ifetch_pkt(NULL), dcache_pkt(NULL), previousCycle(0),
    fetchEvent(this),
    //LACore stuff
    laCacheMasterId(p->system->getMasterId(name() + ".la_cache")),
    laCachePort(name() + ".la_cache_port", this, p->mem_unit_channels),
    laCache(p->laCache), laCacheTLB(p->laCacheTLB), scratchpad(p->scratchpad),
    uniqueReqId(0), laExecUnit(p->laExecUnit)
{
    _status = Idle;

    //create an independent port for each mem_unit to the scratchpad
    for(uint8_t i=0; i<p->mem_unit_channels; ++i){
        scratchMasterIds.push_back(
            p->system->getMasterId(name() + ".scratch" + std::to_string(i))),
        scratchPorts.push_back(ScratchPort(name() + ".scratch_port", this, i));
    }
}

TimingLACoreSimpleCPU::~TimingLACoreSimpleCPU()
{
}

DrainState
TimingLACoreSimpleCPU::drain()
{
    if (switchedOut())
        return DrainState::Drained;

    if (_status == Idle ||
        (_status == BaseLACoreSimpleCPU::Running && isDrained())) {
        DPRINTF(Drain, "No need to drain.\n");
        activeThreads.clear();
        return DrainState::Drained;
    } else {
        DPRINTF(Drain, "Requesting drain.\n");

        // The fetch event can become descheduled if a drain didn't
        // succeed on the first attempt. We need to reschedule it if
        // the CPU is waiting for a microcode routine to complete.
        if (_status == BaseLACoreSimpleCPU::Running && !fetchEvent.scheduled())
            schedule(fetchEvent, clockEdge());

        return DrainState::Draining;
    }
}

void
TimingLACoreSimpleCPU::drainResume()
{
    assert(!fetchEvent.scheduled());
    if (switchedOut())
        return;

    DPRINTF(LACoreSimpleCPU, "Resume\n");
    verifyMemoryMode();

    assert(!threadContexts.empty());

    _status = BaseLACoreSimpleCPU::Idle;

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        if (threadInfo[tid]->thread->status() == ThreadContext::Active) {
            threadInfo[tid]->notIdleFraction = 1;

            activeThreads.push_back(tid);

            _status = BaseLACoreSimpleCPU::Running;

            // Fetch if any threads active
            if (!fetchEvent.scheduled()) {
                schedule(fetchEvent, nextCycle());
            }
        } else {
            threadInfo[tid]->notIdleFraction = 0;
        }
    }

    system->totalNumInsts = 0;
}

bool
TimingLACoreSimpleCPU::tryCompleteDrain()
{
    if (drainState() != DrainState::Draining)
        return false;

    DPRINTF(Drain, "tryCompleteDrain.\n");
    if (!isDrained())
        return false;

    DPRINTF(Drain, "CPU done draining, processing drain event\n");
    signalDrainDone();

    return true;
}

void
TimingLACoreSimpleCPU::switchOut()
{
    LACoreSimpleExecContext& t_info = *threadInfo[curThread];
    M5_VAR_USED LACoreThread* thread = t_info.thread;

    BaseLACoreSimpleCPU::switchOut();

    assert(!fetchEvent.scheduled());
    assert(_status == BaseLACoreSimpleCPU::Running || _status == Idle);
    assert(!t_info.stayAtPC);
    assert(thread->microPC() == 0);

    updateCycleCounts();
}


void
TimingLACoreSimpleCPU::takeOverFrom(BaseCPU *oldCPU)
{
    BaseLACoreSimpleCPU::takeOverFrom(oldCPU);

    previousCycle = curCycle();
}

void
TimingLACoreSimpleCPU::verifyMemoryMode() const
{
    if (!system->isTimingMode()) {
        fatal("The timing CPU requires the memory system to be in "
              "'timing' mode.\n");
    }
}

void
TimingLACoreSimpleCPU::activateContext(ThreadID thread_num)
{
    DPRINTF(LACoreSimpleCPU, "ActivateContext %d\n", thread_num);

    assert(thread_num < numThreads);

    threadInfo[thread_num]->notIdleFraction = 1;
    if (_status == BaseLACoreSimpleCPU::Idle)
        _status = BaseLACoreSimpleCPU::Running;

    // kick things off by initiating the fetch of the next instruction
    if (!fetchEvent.scheduled())
        schedule(fetchEvent, clockEdge(Cycles(0)));

    if (std::find(activeThreads.begin(), activeThreads.end(), thread_num)
         == activeThreads.end()) {
        activeThreads.push_back(thread_num);
    }

    BaseCPU::activateContext(thread_num);
}


void
TimingLACoreSimpleCPU::suspendContext(ThreadID thread_num)
{
    DPRINTF(LACoreSimpleCPU, "SuspendContext %d\n", thread_num);

    assert(thread_num < numThreads);
    activeThreads.remove(thread_num);

    if (_status == Idle)
        return;

    assert(_status == BaseLACoreSimpleCPU::Running);

    threadInfo[thread_num]->notIdleFraction = 0;

    if (activeThreads.empty()) {
        _status = Idle;

        if (fetchEvent.scheduled()) {
            deschedule(fetchEvent);
        }
    }

    BaseCPU::suspendContext(thread_num);
}

bool
TimingLACoreSimpleCPU::handleReadPacket(PacketPtr pkt)
{
    LACoreSimpleExecContext &t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    RequestPtr req = pkt->req;

    // We're about the issues a locked load, so tell the monitor
    // to start caring about this address
    if (pkt->isRead() && pkt->req->isLLSC()) {
        TheISA::handleLockedRead(thread, pkt->req);
    }
    if (req->isMmappedIpr()) {
        Cycles delay = TheISA::handleIprRead(thread->getTC(), pkt);
        new IprEvent(pkt, this, clockEdge(delay));
        _status = DcacheWaitResponse;
        dcache_pkt = NULL;
    } else if (!dcachePort.sendTimingReq(pkt)) {
        _status = DcacheRetry;
        dcache_pkt = pkt;
    } else {
        _status = DcacheWaitResponse;
        // memory system takes ownership of packet
        dcache_pkt = NULL;
    }
    return dcache_pkt == NULL;
}

void
TimingLACoreSimpleCPU::sendData(RequestPtr req, uint8_t *data, uint64_t *res,
                          bool read)
{
    LACoreExecContext &t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    PacketPtr pkt = buildPacket(req, read);
    pkt->dataDynamic<uint8_t>(data);
    if (req->getFlags().isSet(Request::NO_ACCESS)) {
        assert(!dcache_pkt);
        pkt->makeResponse();
        completeDataAccess(pkt);
    } else if (read) {
        handleReadPacket(pkt);
    } else {
        bool do_access = true;  // flag to suppress cache access

        if (req->isLLSC()) {
            do_access = TheISA::handleLockedWrite(thread, req, dcachePort.cacheBlockMask);
        } else if (req->isCondSwap()) {
            assert(res);
            req->setExtraData(*res);
        }

        if (do_access) {
            dcache_pkt = pkt;
            handleWritePacket();
            threadSnoop(pkt, curThread);
        } else {
            _status = DcacheWaitResponse;
            completeDataAccess(pkt);
        }
    }
}

void
TimingLACoreSimpleCPU::sendSplitData(RequestPtr req1, RequestPtr req2,
                               RequestPtr req, uint8_t *data, bool read)
{
    PacketPtr pkt1, pkt2;
    buildSplitPacket(pkt1, pkt2, req1, req2, req, data, read);
    if (req->getFlags().isSet(Request::NO_ACCESS)) {
        assert(!dcache_pkt);
        pkt1->makeResponse();
        completeDataAccess(pkt1);
    } else if (read) {
        SplitFragmentSenderState * send_state =
            dynamic_cast<SplitFragmentSenderState *>(pkt1->senderState);
        if (handleReadPacket(pkt1)) {
            send_state->clearFromParent();
            send_state = dynamic_cast<SplitFragmentSenderState *>(
                    pkt2->senderState);
            if (handleReadPacket(pkt2)) {
                send_state->clearFromParent();
            }
        }
    } else {
        dcache_pkt = pkt1;
        SplitFragmentSenderState * send_state =
            dynamic_cast<SplitFragmentSenderState *>(pkt1->senderState);
        if (handleWritePacket()) {
            send_state->clearFromParent();
            dcache_pkt = pkt2;
            send_state = dynamic_cast<SplitFragmentSenderState *>(
                    pkt2->senderState);
            if (handleWritePacket()) {
                send_state->clearFromParent();
            }
        }
    }
}

void
TimingLACoreSimpleCPU::translationFault(const Fault &fault)
{
    // fault may be NoFault in cases where a fault is suppressed,
    // for instance prefetches.
    updateCycleCounts();

    if (traceData) {
        // Since there was a fault, we shouldn't trace this instruction.
        delete traceData;
        traceData = NULL;
    }

    postExecute();

    advanceInst(fault);
}

PacketPtr
TimingLACoreSimpleCPU::buildPacket(RequestPtr req, bool read)
{
    return read ? Packet::createRead(req) : Packet::createWrite(req);
}

void
TimingLACoreSimpleCPU::buildSplitPacket(PacketPtr &pkt1, PacketPtr &pkt2,
        RequestPtr req1, RequestPtr req2, RequestPtr req,
        uint8_t *data, bool read)
{
    pkt1 = pkt2 = NULL;

    assert(!req1->isMmappedIpr() && !req2->isMmappedIpr());

    if (req->getFlags().isSet(Request::NO_ACCESS)) {
        pkt1 = buildPacket(req, read);
        return;
    }

    pkt1 = buildPacket(req1, read);
    pkt2 = buildPacket(req2, read);

    PacketPtr pkt = new Packet(req, pkt1->cmd.responseCommand());

    pkt->dataDynamic<uint8_t>(data);
    pkt1->dataStatic<uint8_t>(data);
    pkt2->dataStatic<uint8_t>(data + req1->getSize());

    SplitMainSenderState * main_send_state = new SplitMainSenderState;
    pkt->senderState = main_send_state;
    main_send_state->fragments[0] = pkt1;
    main_send_state->fragments[1] = pkt2;
    main_send_state->outstanding = 2;
    pkt1->senderState = new SplitFragmentSenderState(pkt, 0);
    pkt2->senderState = new SplitFragmentSenderState(pkt, 1);
}

Fault
TimingLACoreSimpleCPU::readMem(Addr addr, uint8_t *data,
                         unsigned size, Request::Flags flags)
{
    panic("readMem() is for atomic accesses, and should "
          "never be called on TimingLACoreSimpleCPU.\n");
}

Fault
TimingLACoreSimpleCPU::initiateMemRead(Addr addr, unsigned size,
                                 Request::Flags flags)
{
    LACoreSimpleExecContext &t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    Fault fault;
    const int asid = 0;
    const Addr pc = thread->instAddr();
    unsigned block_size = cacheLineSize();
    BaseTLB::Mode mode = BaseTLB::Read;

    if (traceData)
        traceData->setMem(addr, size, flags);

    RequestPtr req = new Request(asid, addr, size, flags, dataMasterId(), pc,
                                 thread->contextId());

    req->taskId(taskId());

    Addr split_addr = roundDown(addr + size - 1, block_size);
    assert(split_addr <= addr || split_addr - addr < block_size);

    _status = DTBWaitResponse;
    if (split_addr > addr) {
        RequestPtr req1, req2;
        assert(!req->isLLSC() && !req->isSwap());
        req->splitOnVaddr(split_addr, req1, req2);

        WholeTranslationState *state =
            new WholeTranslationState(req, req1, req2, new uint8_t[size],
                                      NULL, mode);
        DataTranslation<TimingLACoreSimpleCPU *> *trans1 =
            new DataTranslation<TimingLACoreSimpleCPU *>(this, state, 0);
        DataTranslation<TimingLACoreSimpleCPU *> *trans2 =
            new DataTranslation<TimingLACoreSimpleCPU *>(this, state, 1);

        thread->dtb->translateTiming(req1, thread->getTC(), trans1, mode);
        thread->dtb->translateTiming(req2, thread->getTC(), trans2, mode);
    } else {
        WholeTranslationState *state =
            new WholeTranslationState(req, new uint8_t[size], NULL, mode);
        DataTranslation<TimingLACoreSimpleCPU *> *translation
            = new DataTranslation<TimingLACoreSimpleCPU *>(this, state);
        thread->dtb->translateTiming(req, thread->getTC(), translation, mode);
    }

    return NoFault;
}

bool
TimingLACoreSimpleCPU::handleWritePacket()
{
    LACoreSimpleExecContext &t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    RequestPtr req = dcache_pkt->req;
    if (req->isMmappedIpr()) {
        Cycles delay = TheISA::handleIprWrite(thread->getTC(), dcache_pkt);
        new IprEvent(dcache_pkt, this, clockEdge(delay));
        _status = DcacheWaitResponse;
        dcache_pkt = NULL;
    } else if (!dcachePort.sendTimingReq(dcache_pkt)) {
        _status = DcacheRetry;
    } else {
        _status = DcacheWaitResponse;
        // memory system takes ownership of packet
        dcache_pkt = NULL;
    }
    return dcache_pkt == NULL;
}

Fault
TimingLACoreSimpleCPU::writeMem(uint8_t *data, unsigned size,
                          Addr addr, Request::Flags flags, uint64_t *res)
{
    LACoreSimpleExecContext &t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    uint8_t *newData = new uint8_t[size];
    const int asid = 0;
    const Addr pc = thread->instAddr();
    unsigned block_size = cacheLineSize();
    BaseTLB::Mode mode = BaseTLB::Write;

    if (data == NULL) {
        assert(flags & Request::CACHE_BLOCK_ZERO);
        // This must be a cache block cleaning request
        memset(newData, 0, size);
    } else {
        memcpy(newData, data, size);
    }

    if (traceData)
        traceData->setMem(addr, size, flags);

    RequestPtr req = new Request(asid, addr, size, flags, dataMasterId(), pc,
                                 thread->contextId());

    req->taskId(taskId());

    Addr split_addr = roundDown(addr + size - 1, block_size);
    assert(split_addr <= addr || split_addr - addr < block_size);

    _status = DTBWaitResponse;
    if (split_addr > addr) {
        RequestPtr req1, req2;
        assert(!req->isLLSC() && !req->isSwap());
        req->splitOnVaddr(split_addr, req1, req2);

        WholeTranslationState *state =
            new WholeTranslationState(req, req1, req2, newData, res, mode);
        DataTranslation<TimingLACoreSimpleCPU *> *trans1 =
            new DataTranslation<TimingLACoreSimpleCPU *>(this, state, 0);
        DataTranslation<TimingLACoreSimpleCPU *> *trans2 =
            new DataTranslation<TimingLACoreSimpleCPU *>(this, state, 1);

        thread->dtb->translateTiming(req1, thread->getTC(), trans1, mode);
        thread->dtb->translateTiming(req2, thread->getTC(), trans2, mode);
    } else {
        WholeTranslationState *state =
            new WholeTranslationState(req, newData, res, mode);
        DataTranslation<TimingLACoreSimpleCPU *> *translation =
            new DataTranslation<TimingLACoreSimpleCPU *>(this, state);
        thread->dtb->translateTiming(req, thread->getTC(), translation, mode);
    }

    // Translation faults will be returned via finishTranslation()
    return NoFault;
}

void
TimingLACoreSimpleCPU::threadSnoop(PacketPtr pkt, ThreadID sender)
{
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        if (tid != sender) {
            if (getCpuAddrMonitor(tid)->doMonitor(pkt)) {
                wakeup(tid);
            }
            TheISA::handleLockedSnoop(threadInfo[tid]->thread, pkt,
                    dcachePort.cacheBlockMask);
        }
    }
}

void
TimingLACoreSimpleCPU::finishTranslation(WholeTranslationState *state)
{
    _status = BaseLACoreSimpleCPU::Running;

    if (state->getFault() != NoFault) {
        if (state->isPrefetch()) {
            state->setNoFault();
        }
        delete [] state->data;
        state->deleteReqs();
        translationFault(state->getFault());
    } else {
        if (!state->isSplit) {
            sendData(state->mainReq, state->data, state->res,
                     state->mode == BaseTLB::Read);
        } else {
            sendSplitData(state->sreqLow, state->sreqHigh, state->mainReq,
                          state->data, state->mode == BaseTLB::Read);
        }
    }

    delete state;
}


void
TimingLACoreSimpleCPU::fetch()
{
    // Change thread if multi-threaded
    swapActiveThread();

    LACoreSimpleExecContext &t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    DPRINTF(LACoreSimpleCPU, "Fetch\n");

    if (!curStaticInst || !curStaticInst->isDelayedCommit()) {
        checkForInterrupts();
        checkPcEventQueue();
    }

    // We must have just got suspended by a PC event
    if (_status == Idle)
        return;

    TheISA::PCState pcState = thread->pcState();
    bool needToFetch = !isRomMicroPC(pcState.microPC()) &&
                       !curMacroStaticInst;

    if (needToFetch) {
        _status = BaseLACoreSimpleCPU::Running;
        Request *ifetch_req = new Request();
        ifetch_req->taskId(taskId());
        ifetch_req->setContext(thread->contextId());
        setupFetchRequest(ifetch_req);
        DPRINTF(LACoreSimpleCPU, "Translating address %#x\n",
            ifetch_req->getVaddr());
        thread->itb->translateTiming(ifetch_req, thread->getTC(),
                &fetchTranslation, BaseTLB::Execute);
    } else {
        _status = IcacheWaitResponse;
        completeIfetch(NULL);

        updateCycleCounts();
    }
}


void
TimingLACoreSimpleCPU::sendFetch(const Fault &fault, RequestPtr req,
                           ThreadContext *tc)
{
    if (fault == NoFault) {
        DPRINTF(LACoreSimpleCPU, "Sending fetch for addr %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());
        ifetch_pkt = new Packet(req, MemCmd::ReadReq);
        ifetch_pkt->dataStatic(&inst);
        DPRINTF(LACoreSimpleCPU, " -- pkt addr: %#x\n", ifetch_pkt->getAddr());

        if (!icachePort.sendTimingReq(ifetch_pkt)) {
            // Need to wait for retry
            _status = IcacheRetry;
        } else {
            // Need to wait for cache to respond
            _status = IcacheWaitResponse;
            // ownership of packet transferred to memory system
            ifetch_pkt = NULL;
        }
    } else {
        DPRINTF(LACoreSimpleCPU, "Translation of addr %#x faulted\n",
            req->getVaddr());
        delete req;
        // fetch fault: advance directly to next instruction (fault handler)
        _status = BaseLACoreSimpleCPU::Running;
        advanceInst(fault);
    }

    updateCycleCounts();
}


void
TimingLACoreSimpleCPU::advanceInst(const Fault &fault)
{
    LACoreSimpleExecContext &t_info = *threadInfo[curThread];

    if (_status == Faulting)
        return;

    if (fault != NoFault) {
        advancePC(fault);
        DPRINTF(LACoreSimpleCPU, "Fault occured, scheduling fetch event\n");
        reschedule(fetchEvent, clockEdge(), true);
        _status = Faulting;
        return;
    }


    if (!t_info.stayAtPC)
        advancePC(fault);

    if (tryCompleteDrain())
            return;

    if (_status == BaseLACoreSimpleCPU::Running) {
        // kick off fetch of next instruction... callback from icache
        // response will cause that instruction to be executed,
        // keeping the CPU running.
        fetch();
    }
}


void
TimingLACoreSimpleCPU::completeIfetch(PacketPtr pkt)
{
    LACoreSimpleExecContext& t_info = *threadInfo[curThread];

    DPRINTF(LACoreSimpleCPU, "Complete ICache Fetch for addr %#x\n", pkt ?
            pkt->getAddr() : 0);

    // received a response from the icache: execute the received
    // instruction
    assert(!pkt || !pkt->isError());
    assert(_status == IcacheWaitResponse);

    _status = BaseLACoreSimpleCPU::Running;

    updateCycleCounts();

    if (pkt)
        pkt->req->setAccessLatency();

    LAInsn *la_insn = nullptr;

    preExecute();
    if (curStaticInst && curStaticInst->isMemRef()) {
        // load or store: just send to dcache
        Fault fault = curStaticInst->initiateAcc(&t_info, traceData);

        // If we're not running now the instruction will complete in a dcache
        // response callback or the instruction faulted and has started an
        // ifetch
        if (_status == BaseLACoreSimpleCPU::Running) {
            if (fault != NoFault && traceData) {
                // If there was a fault, we shouldn't trace this instruction.
                delete traceData;
                traceData = NULL;
            }

            postExecute();
            // @todo remove me after debugging with legion done
            if (curStaticInst && (!curStaticInst->isMicroop() ||
                        curStaticInst->isFirstMicroop()))
                instCnt++;
            advanceInst(fault);
        }
    } else if (curStaticInst) {
        la_insn = dynamic_cast<LAInsn*>(curStaticInst.get());
        if (curStaticInst && la_insn != nullptr) {
            delete pkt->req;
            delete pkt;
            startExecuteLAInsn(*la_insn, &t_info);
        } else {
        // non-memory instruction: execute completely now
            Fault fault = curStaticInst->execute(&t_info, traceData);

            // keep an instruction count
            if (fault == NoFault)
                countInst();
            else if (traceData && !DTRACE(ExecFaulting)) {
                delete traceData;
                traceData = NULL;
            }

            postExecute();
            // @todo remove me after debugging with legion done
            if (curStaticInst && (!curStaticInst->isMicroop() ||
                    curStaticInst->isFirstMicroop()))
                instCnt++;
            advanceInst(fault);
        }
    } else {
        advanceInst(NoFault);
    }

    //STEFFL: don't delete pkt and req till after lacore done
    if (pkt && (la_insn == nullptr)) {
        delete pkt->req;
        delete pkt;
    }
}

void
TimingLACoreSimpleCPU::IcachePort::ITickEvent::process()
{
    cpu->completeIfetch(pkt);
}

bool
TimingLACoreSimpleCPU::IcachePort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(LACoreSimpleCPU, "Received fetch response %#x\n", pkt->getAddr());
    // we should only ever see one response per cycle since we only
    // issue a new request once this response is sunk
    assert(!tickEvent.scheduled());
    // delay processing of returned data until next CPU clock edge
    tickEvent.schedule(pkt, cpu->clockEdge());

    return true;
}

void
TimingLACoreSimpleCPU::IcachePort::recvReqRetry()
{
    // we shouldn't get a retry unless we have a packet that we're
    // waiting to transmit
    assert(cpu->ifetch_pkt != NULL);
    assert(cpu->_status == IcacheRetry);
    PacketPtr tmp = cpu->ifetch_pkt;
    if (sendTimingReq(tmp)) {
        cpu->_status = IcacheWaitResponse;
        cpu->ifetch_pkt = NULL;
    }
}

void
TimingLACoreSimpleCPU::completeDataAccess(PacketPtr pkt)
{
    // received a response from the dcache: complete the load or store
    // instruction
    assert(!pkt->isError());
    assert(_status == DcacheWaitResponse || _status == DTBWaitResponse ||
           pkt->req->getFlags().isSet(Request::NO_ACCESS));

    pkt->req->setAccessLatency();

    updateCycleCounts();

    if (pkt->senderState) {
        SplitFragmentSenderState * send_state =
            dynamic_cast<SplitFragmentSenderState *>(pkt->senderState);
        assert(send_state);
        delete pkt->req;
        delete pkt;
        PacketPtr big_pkt = send_state->bigPkt;
        delete send_state;

        SplitMainSenderState * main_send_state =
            dynamic_cast<SplitMainSenderState *>(big_pkt->senderState);
        assert(main_send_state);
        // Record the fact that this packet is no longer outstanding.
        assert(main_send_state->outstanding != 0);
        main_send_state->outstanding--;

        if (main_send_state->outstanding) {
            return;
        } else {
            delete main_send_state;
            big_pkt->senderState = NULL;
            pkt = big_pkt;
        }
    }

    _status = BaseLACoreSimpleCPU::Running;

    Fault fault = curStaticInst->completeAcc(pkt, threadInfo[curThread],
                                             traceData);

    // keep an instruction count
    if (fault == NoFault)
        countInst();
    else if (traceData) {
        // If there was a fault, we shouldn't trace this instruction.
        delete traceData;
        traceData = NULL;
    }

    delete pkt->req;
    delete pkt;

    postExecute();

    advanceInst(fault);
}

void
TimingLACoreSimpleCPU::updateCycleCounts()
{
    const Cycles delta(curCycle() - previousCycle);

    numCycles += delta;
    ppCycles->notify(delta);

    previousCycle = curCycle();
}

void
TimingLACoreSimpleCPU::DcachePort::recvTimingSnoopReq(PacketPtr pkt)
{
    for (ThreadID tid = 0; tid < cpu->numThreads; tid++) {
        if (cpu->getCpuAddrMonitor(tid)->doMonitor(pkt)) {
            cpu->wakeup(tid);
        }
    }

    // Making it uniform across all CPUs:
    // The CPUs need to be woken up only on an invalidation packet
    // (when using caches)
    // or on an incoming write packet (when not using caches)
    // It is not necessary to wake up the processor on all incoming packets
    if (pkt->isInvalidate() || pkt->isWrite()) {
        for (auto &t_info : cpu->threadInfo) {
            TheISA::handleLockedSnoop(t_info->thread, pkt, cacheBlockMask);
        }
    }
}

void
TimingLACoreSimpleCPU::DcachePort::recvFunctionalSnoop(PacketPtr pkt)
{
    for (ThreadID tid = 0; tid < cpu->numThreads; tid++) {
        if (cpu->getCpuAddrMonitor(tid)->doMonitor(pkt)) {
            cpu->wakeup(tid);
        }
    }
}

bool
TimingLACoreSimpleCPU::DcachePort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(LACoreSimpleCPU, "Received load/store response %#x\n",
        pkt->getAddr());

    // The timing CPU is not really ticked, instead it relies on the
    // memory system (fetch and load/store) to set the pace.
    if (!tickEvent.scheduled()) {
        // Delay processing of returned data until next CPU clock edge
        tickEvent.schedule(pkt, cpu->clockEdge());
        return true;
    } else {
        // In the case of a split transaction and a cache that is
        // faster than a CPU we could get two responses in the
        // same tick, delay the second one
        if (!retryRespEvent.scheduled())
            cpu->schedule(retryRespEvent, cpu->clockEdge(Cycles(1)));
        return false;
    }
}

void
TimingLACoreSimpleCPU::DcachePort::DTickEvent::process()
{
    cpu->completeDataAccess(pkt);
}

void
TimingLACoreSimpleCPU::DcachePort::recvReqRetry()
{
    // we shouldn't get a retry unless we have a packet that we're
    // waiting to transmit
    assert(cpu->dcache_pkt != NULL);
    assert(cpu->_status == DcacheRetry);
    PacketPtr tmp = cpu->dcache_pkt;
    if (tmp->senderState) {
        // This is a packet from a split access.
        SplitFragmentSenderState * send_state =
            dynamic_cast<SplitFragmentSenderState *>(tmp->senderState);
        assert(send_state);
        PacketPtr big_pkt = send_state->bigPkt;

        SplitMainSenderState * main_send_state =
            dynamic_cast<SplitMainSenderState *>(big_pkt->senderState);
        assert(main_send_state);

        if (sendTimingReq(tmp)) {
            // If we were able to send without retrying, record that fact
            // and try sending the other fragment.
            send_state->clearFromParent();
            int other_index = main_send_state->getPendingFragment();
            if (other_index > 0) {
                tmp = main_send_state->fragments[other_index];
                cpu->dcache_pkt = tmp;
                if ((big_pkt->isRead() && cpu->handleReadPacket(tmp)) ||
                        (big_pkt->isWrite() && cpu->handleWritePacket())) {
                    main_send_state->fragments[other_index] = NULL;
                }
            } else {
                cpu->_status = DcacheWaitResponse;
                // memory system takes ownership of packet
                cpu->dcache_pkt = NULL;
            }
        }
    } else if (sendTimingReq(tmp)) {
        cpu->_status = DcacheWaitResponse;
        // memory system takes ownership of packet
        cpu->dcache_pkt = NULL;
    }
}

TimingLACoreSimpleCPU::IprEvent::IprEvent(Packet *_pkt, TimingLACoreSimpleCPU *_cpu,
    Tick t)
    : pkt(_pkt), cpu(_cpu)
{
    cpu->schedule(this, t);
}

void
TimingLACoreSimpleCPU::IprEvent::process()
{
    cpu->completeDataAccess(pkt);
}

const char *
TimingLACoreSimpleCPU::IprEvent::description() const
{
    return "Timing Simple CPU Delay IPR event";
}


void
TimingLACoreSimpleCPU::printAddr(Addr a)
{
    dcachePort.printAddr(a);
}


////////////////////////////////////////////////////////////////////////
//
//  TimingLACoreSimpleCPU Simulation Object
//
TimingLACoreSimpleCPU *
TimingLACoreSimpleCPUParams::create()
{
    return new TimingLACoreSimpleCPU(this);
}

//=============================================================================
// Port Functions
//=============================================================================

BaseMasterPort &
TimingLACoreSimpleCPU::getMasterPort(const string &if_name, PortID idx)
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

TimingLACoreSimpleCPU::LACachePort::LACachePort(
    const std::string& name, TimingLACoreSimpleCPU* cpu, uint8_t channels) :
    MasterPort(name, cpu), cpu(cpu)
{
    //create the queues for each of the channels to the LACache
    for (uint8_t i=0; i<channels; ++i) {
        laCachePktQs.push_back(std::deque<PacketPtr>());
    }
}

TimingLACoreSimpleCPU::LACachePort::~LACachePort()
{
}

// submits a request for translation to the execCacheTlb
// MEMCPY data if present, so caller must deallocate it himself!
bool
TimingLACoreSimpleCPU::LACachePort::startTranslation(Addr addr, uint8_t *data,
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
TimingLACoreSimpleCPU::LACachePort::sendTimingReadReq(Addr addr, uint64_t size,
    ThreadContext *tc, uint64_t req_id, uint8_t channel)
{
    return startTranslation(addr, nullptr, size, BaseTLB::Read, tc, req_id,
        channel);
}

bool
TimingLACoreSimpleCPU::LACachePort::sendTimingWriteReq(Addr addr,
    uint8_t *data, uint64_t size, ThreadContext *tc, uint64_t req_id,
    uint8_t channel)
{
    return startTranslation(addr, data, size, BaseTLB::Write, tc, req_id,
        channel);
}

bool
TimingLACoreSimpleCPU::LACachePort::recvTimingResp(PacketPtr pkt)
{
    LACorePacketPtr la_pkt = dynamic_cast<LACorePacketPtr>(pkt);
    assert(la_pkt != nullptr);
    cpu->recvTimingResp(la_pkt);
    return true;
}

void
TimingLACoreSimpleCPU::LACachePort::recvReqRetry()
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

TimingLACoreSimpleCPU::ScratchPort::ScratchPort(const std::string& name,
    TimingLACoreSimpleCPU* cpu, uint64_t channel) :
    MasterPort(name, cpu), cpu(cpu), channel(channel)
{
}

TimingLACoreSimpleCPU::ScratchPort::~ScratchPort()
{
}

bool
TimingLACoreSimpleCPU::ScratchPort::sendTimingReadReq(Addr addr, uint64_t size,
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
TimingLACoreSimpleCPU::ScratchPort::sendTimingWriteReq(Addr addr, uint8_t *data, 
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
TimingLACoreSimpleCPU::ScratchPort::recvTimingResp(PacketPtr pkt)
{
    LACorePacketPtr la_pkt = dynamic_cast<LACorePacketPtr>(pkt);
    assert(la_pkt != nullptr);
    cpu->recvTimingResp(la_pkt);
    return true;
}

void
TimingLACoreSimpleCPU::ScratchPort::recvReqRetry()
{
    //do nothing, caller will manually resend request
}
   
//=========================================================================
//LACore handle timing responses from scratchpad and memory
//========================================================================= 

void
TimingLACoreSimpleCPU::recvTimingResp(LACorePacketPtr la_pkt)
{
    //update cycle count
    updateCycleCounts();

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
TimingLACoreSimpleCPU::writeLAMemAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    panic("TimingLACoreSimpleCPU::writeLAMemAtomic should not be called");
}

Fault 
TimingLACoreSimpleCPU::writeLAScratchAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    panic("TimingLACoreSimpleCPU::writeLAScratchAtomic should not be called");
}

Fault 
TimingLACoreSimpleCPU::readLAMemAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    panic("TimingLACoreSimpleCPU::readLAMemAtomic should not be called");
}

Fault 
TimingLACoreSimpleCPU::readLAScratchAtomic(Addr addr, uint8_t *data,
    uint64_t size, ThreadContext *tc)
{
    panic("TimingLACoreSimpleCPU::readLAScratchAtomic should not be called");
}

//=========================================================================
//LACore TIMING Memory and Scratchpad interface
//=========================================================================

//
// NOTE: does NOT memcpy data, the ports do that!
//
bool
TimingLACoreSimpleCPU::writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size,
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
TimingLACoreSimpleCPU::writeLAScratchTiming(Addr addr, uint8_t *data,
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
TimingLACoreSimpleCPU::readLAMemTiming(Addr addr, uint8_t size, 
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
TimingLACoreSimpleCPU::readLAScratchTiming(Addr addr, uint8_t size,
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

//============================================================================
// All LACore Ops
//============================================================================

void
TimingLACoreSimpleCPU::startExecuteLAInsn(LAInsn& insn,
    LACoreSimpleExecContext *xc)
{
    assert(!laExecUnit->isOccupied());
    laExecUnit->issue(insn, xc, [this](Fault f){ finishExecuteLAInsn(f); });
}

void
TimingLACoreSimpleCPU::finishExecuteLAInsn(Fault fault)
{
    // keep an instruction count
    if (fault == NoFault) {
        countInst();
    } else if (traceData && !DTRACE(ExecFaulting)) {
        delete traceData;
        traceData = NULL;
    }

    postExecute();
    // @todo remove me after debugging with legion done
    if (curStaticInst && (!curStaticInst->isMicroop() ||
            curStaticInst->isFirstMicroop()))
        instCnt++;
    advanceInst(fault);
}

