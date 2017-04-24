/*
 * Copyright 2014 Google, Inc.
 * Copyright (c) 2012-2013,2015 ARM Limited
 * All rights reserved.
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

#include "cpu/la_core/simple/atomic.hh"

#include <algorithm>
#include <cstring>

#include "arch/locked_mem.hh"
#include "arch/mmapped_ipr.hh"
#include "arch/utility.hh"
#include "base/bigint.hh"
#include "base/output.hh"
#include "config/the_isa.hh"
#include "cpu/exetrace.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"
#include "cpu/la_core/la_mem_unit/atomic_unit.hh"
#include "cpu/la_core/simple/exec_context.hh"
#include "cpu/la_core/scratchpad/atomic.hh"
#include "cpu/simple_thread.hh"
#include "debug/Drain.hh"
#include "debug/ExecFaulting.hh"
#include "debug/LACoreSimpleCPU.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "mem/physical.hh"
#include "params/AtomicLACoreSimpleCPU.hh"
#include "sim/faults.hh"
#include "sim/system.hh"
#include "sim/full_system.hh"

using namespace std;
using namespace TheISA;

AtomicLACoreSimpleCPU::TickEvent::TickEvent(AtomicLACoreSimpleCPU *c)
    : Event(CPU_Tick_Pri), cpu(c)
{
}


void
AtomicLACoreSimpleCPU::TickEvent::process()
{
    cpu->tick();
}

const char *
AtomicLACoreSimpleCPU::TickEvent::description() const
{
    return "AtomicLACoreSimpleCPU tick";
}

void
AtomicLACoreSimpleCPU::init()
{
    BaseLACoreSimpleCPU::init();

    int cid = threadContexts[0]->contextId();
    ifetch_req.setContext(cid);
    data_read_req.setContext(cid);
    data_write_req.setContext(cid);
}

AtomicLACoreSimpleCPU::AtomicLACoreSimpleCPU(AtomicLACoreSimpleCPUParams *p)
    : BaseLACoreSimpleCPU(p), tickEvent(this), width(p->width), locked(false),
      simulate_data_stalls(p->simulate_data_stalls),
      simulate_inst_stalls(p->simulate_inst_stalls),
      icachePort(name() + ".icache_port", this),
      dcachePort(name() + ".dcache_port", this),
      fastmem(p->fastmem), dcache_access(false), dcache_latency(0),
      ppCommit(nullptr),
      scratchpad(p->scratchSize)
{
    _status = Idle;
}


AtomicLACoreSimpleCPU::~AtomicLACoreSimpleCPU()
{
    if (tickEvent.scheduled()) {
        deschedule(tickEvent);
    }
}

DrainState
AtomicLACoreSimpleCPU::drain()
{
    if (switchedOut())
        return DrainState::Drained;

    if (!isDrained()) {
        DPRINTF(Drain, "Requesting drain.\n");
        return DrainState::Draining;
    } else {
        if (tickEvent.scheduled())
            deschedule(tickEvent);

        activeThreads.clear();
        DPRINTF(Drain, "Not executing microcode, no need to drain.\n");
        return DrainState::Drained;
    }
}

void
AtomicLACoreSimpleCPU::threadSnoop(PacketPtr pkt, ThreadID sender)
{
    DPRINTF(LACoreSimpleCPU, "received snoop pkt for addr:%#x %s\n", 
        pkt->getAddr(),pkt->cmdString());

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        if (tid != sender) {
            if (getCpuAddrMonitor(tid)->doMonitor(pkt)) {
                wakeup(tid);
            }

            TheISA::handleLockedSnoop(threadInfo[tid]->thread,
                                      pkt, dcachePort.cacheBlockMask);
        }
    }
}

void
AtomicLACoreSimpleCPU::drainResume()
{
    assert(!tickEvent.scheduled());
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

            // Tick if any threads active
            if (!tickEvent.scheduled()) {
                schedule(tickEvent, nextCycle());
            }
        } else {
            threadInfo[tid]->notIdleFraction = 0;
        }
    }
}

bool
AtomicLACoreSimpleCPU::tryCompleteDrain()
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
AtomicLACoreSimpleCPU::switchOut()
{
    BaseLACoreSimpleCPU::switchOut();

    assert(!tickEvent.scheduled());
    assert(_status == BaseLACoreSimpleCPU::Running || _status == Idle);
    assert(isDrained());
}


void
AtomicLACoreSimpleCPU::takeOverFrom(BaseCPU *oldCPU)
{
    BaseLACoreSimpleCPU::takeOverFrom(oldCPU);

    // The tick event should have been descheduled by drain()
    assert(!tickEvent.scheduled());
}

void
AtomicLACoreSimpleCPU::verifyMemoryMode() const
{
    if (!system->isAtomicMode()) {
        fatal("The atomic CPU requires the memory system to be in "
              "'atomic' mode.\n");
    }
}

void
AtomicLACoreSimpleCPU::activateContext(ThreadID thread_num)
{
    DPRINTF(LACoreSimpleCPU, "ActivateContext %d\n", thread_num);

    assert(thread_num < numThreads);

    threadInfo[thread_num]->notIdleFraction = 1;
    Cycles delta = ticksToCycles(threadInfo[thread_num]->thread->lastActivate -
                                 threadInfo[thread_num]->thread->lastSuspend);
    numCycles += delta;
    ppCycles->notify(delta);

    if (!tickEvent.scheduled()) {
        //Make sure ticks are still on multiples of cycles
        schedule(tickEvent, clockEdge(Cycles(0)));
    }
    _status = BaseLACoreSimpleCPU::Running;
    if (std::find(activeThreads.begin(), activeThreads.end(), thread_num)
        == activeThreads.end()) {
        activeThreads.push_back(thread_num);
    }

    BaseCPU::activateContext(thread_num);
}


void
AtomicLACoreSimpleCPU::suspendContext(ThreadID thread_num)
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

        if (tickEvent.scheduled()) {
            deschedule(tickEvent);
        }
    }

    BaseCPU::suspendContext(thread_num);
}


Tick
AtomicLACoreSimpleCPU::AtomicCPUDPort::recvAtomicSnoop(PacketPtr pkt)
{
    DPRINTF(LACoreSimpleCPU, "received snoop pkt for addr:%#x %s\n", 
        pkt->getAddr(), pkt->cmdString());

    // X86 ISA: Snooping an invalidation for monitor/mwait
    AtomicLACoreSimpleCPU *cpu = (AtomicLACoreSimpleCPU *)(&owner);

    for (ThreadID tid = 0; tid < cpu->numThreads; tid++) {
        if (cpu->getCpuAddrMonitor(tid)->doMonitor(pkt)) {
            cpu->wakeup(tid);
        }
    }

    // if snoop invalidates, release any associated locks
    // When run without caches, Invalidation packets will not be received
    // hence we must check if the incoming packets are writes and wakeup
    // the processor accordingly
    if (pkt->isInvalidate() || pkt->isWrite()) {
        DPRINTF(LACoreSimpleCPU, "received invalidation for addr:%#x\n",
                pkt->getAddr());
        for (auto &t_info : cpu->threadInfo) {
            TheISA::handleLockedSnoop(t_info->thread, pkt, cacheBlockMask);
        }
    }

    return 0;
}

void
AtomicLACoreSimpleCPU::AtomicCPUDPort::recvFunctionalSnoop(PacketPtr pkt)
{
    DPRINTF(LACoreSimpleCPU, "received snoop pkt for addr:%#x %s\n", 
        pkt->getAddr(), pkt->cmdString());

    // X86 ISA: Snooping an invalidation for monitor/mwait
    AtomicLACoreSimpleCPU *cpu = (AtomicLACoreSimpleCPU *)(&owner);
    for (ThreadID tid = 0; tid < cpu->numThreads; tid++) {
        if (cpu->getCpuAddrMonitor(tid)->doMonitor(pkt)) {
            cpu->wakeup(tid);
        }
    }

    // if snoop invalidates, release any associated locks
    if (pkt->isInvalidate()) {
        DPRINTF(LACoreSimpleCPU, "received invalidation for addr:%#x\n",
                pkt->getAddr());
        for (auto &t_info : cpu->threadInfo) {
            TheISA::handleLockedSnoop(t_info->thread, pkt, cacheBlockMask);
        }
    }
}

Fault
AtomicLACoreSimpleCPU::readMem(Addr addr, uint8_t * data, unsigned size,
                               Request::Flags flags)
{
    LACoreSimpleExecContext& t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    // use the CPU's statically allocated read request and packet objects
    Request *req = &data_read_req;

    if (traceData)
        traceData->setMem(addr, size, flags);

    //The size of the data we're trying to read.
    int fullSize = size;

    //The address of the second part of this access if it needs to be split
    //across a cache line boundary.
    Addr secondAddr = roundDown(addr + size - 1, cacheLineSize());

    if (secondAddr > addr)
        size = secondAddr - addr;

    dcache_latency = 0;

    req->taskId(taskId());
    while (1) {
        req->setVirt(0, addr, size, flags, dataMasterId(), 
            thread->pcState().instAddr());

        // translate to physical address
        Fault fault = thread->dtb->translateAtomic(req, thread->getTC(),
                                                          BaseTLB::Read);

        // Now do the access.
        if (fault == NoFault && !req->getFlags().isSet(Request::NO_ACCESS)) {
            Packet pkt(req, Packet::makeReadCmd(req));
            pkt.dataStatic(data);

            if (req->isMmappedIpr())
                dcache_latency += TheISA::handleIprRead(thread->getTC(), &pkt);
            else {
                if (fastmem && system->isMemAddr(pkt.getAddr()))
                    system->getPhysMem().access(&pkt);
                else
                    dcache_latency += dcachePort.sendAtomic(&pkt);
            }
            dcache_access = true;

            assert(!pkt.isError());

            if (req->isLLSC()) {
                TheISA::handleLockedRead(thread, req);
            }
        }

        //If there's a fault, return it
        if (fault != NoFault) {
            if (req->isPrefetch()) {
                return NoFault;
            } else {
                return fault;
            }
        }

        //If we don't need to access a second cache line, stop now.
        if (secondAddr <= addr)
        {
            if (req->isLockedRMW() && fault == NoFault) {
                assert(!locked);
                locked = true;
            }

            return fault;
        }

        /*
         * Set up for accessing the second cache line.
         */

        //Move the pointer we're reading into to the correct location.
        data += size;
        //Adjust the size to get the remaining bytes.
        size = addr + fullSize - secondAddr;
        //And access the right address.
        addr = secondAddr;
    }
}

Fault
AtomicLACoreSimpleCPU::initiateMemRead(Addr addr, unsigned size,
                                 Request::Flags flags)
{
    panic("initiateMemRead() is for timing accesses, and should "
          "never be called on AtomicLACoreSimpleCPU.\n");
}

Fault
AtomicLACoreSimpleCPU::writeMem(uint8_t *data, unsigned size, Addr addr,
                          Request::Flags flags, uint64_t *res)
{
    LACoreSimpleExecContext& t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;
    static uint8_t zero_array[64] = {};

    if (data == NULL) {
        assert(size <= 64);
        assert(flags & Request::CACHE_BLOCK_ZERO);
        // This must be a cache block cleaning request
        data = zero_array;
    }

    // use the CPU's statically allocated write request and packet objects
    Request *req = &data_write_req;

    if (traceData)
        traceData->setMem(addr, size, flags);

    //The size of the data we're trying to read.
    int fullSize = size;

    //The address of the second part of this access if it needs to be split
    //across a cache line boundary.
    Addr secondAddr = roundDown(addr + size - 1, cacheLineSize());

    if (secondAddr > addr)
        size = secondAddr - addr;

    dcache_latency = 0;

    req->taskId(taskId());
    while (1) {
        req->setVirt(0, addr, size, flags, dataMasterId(), 
            thread->pcState().instAddr());

        // translate to physical address
        Fault fault = thread->dtb->translateAtomic(req, thread->getTC(), 
            BaseTLB::Write);

        // Now do the access.
        if (fault == NoFault) {
            MemCmd cmd = MemCmd::WriteReq; // default
            bool do_access = true;  // flag to suppress cache access

            if (req->isLLSC()) {
                cmd = MemCmd::StoreCondReq;
                do_access = TheISA::handleLockedWrite(thread, req, 
                    dcachePort.cacheBlockMask);
            } else if (req->isSwap()) {
                cmd = MemCmd::SwapReq;
                if (req->isCondSwap()) {
                    assert(res);
                    req->setExtraData(*res);
                }
            }

            if (do_access && !req->getFlags().isSet(Request::NO_ACCESS)) {
                Packet pkt = Packet(req, cmd);
                pkt.dataStatic(data);

                if (req->isMmappedIpr()) {
                    dcache_latency +=
                        TheISA::handleIprWrite(thread->getTC(), &pkt);
                } else {
                    if (fastmem && system->isMemAddr(pkt.getAddr()))
                        system->getPhysMem().access(&pkt);
                    else
                        dcache_latency += dcachePort.sendAtomic(&pkt);

                    // Notify other threads on this CPU of write
                    threadSnoop(&pkt, curThread);
                }
                dcache_access = true;
                assert(!pkt.isError());

                if (req->isSwap()) {
                    assert(res);
                    memcpy(res, pkt.getConstPtr<uint8_t>(), fullSize);
                }
            }

            if (res && !req->isSwap()) {
                *res = req->getExtraData();
            }
        }

        //If there's a fault or we don't need to access a second cache line,
        //stop now.
        if (fault != NoFault || secondAddr <= addr)
        {
            if (req->isLockedRMW() && fault == NoFault) {
                assert(locked);
                locked = false;
            }


            if (fault != NoFault && req->isPrefetch()) {
                return NoFault;
            } else {
                return fault;
            }
        }

        /*
         * Set up for accessing the second cache line.
         */

        //Move the pointer we're reading into to the correct location.
        data += size;
        //Adjust the size to get the remaining bytes.
        size = addr + fullSize - secondAddr;
        //And access the right address.
        addr = secondAddr;
    }
}


void
AtomicLACoreSimpleCPU::tick()
{
    DPRINTF(LACoreSimpleCPU, "Tick\n");

    // Change thread if multi-threaded
    swapActiveThread();

    // Set memroy request ids to current thread
    if (numThreads > 1) {
        ContextID cid = threadContexts[curThread]->contextId();

        ifetch_req.setContext(cid);
        data_read_req.setContext(cid);
        data_write_req.setContext(cid);
    }

    LACoreSimpleExecContext& t_info = *threadInfo[curThread];
    LACoreThread* thread = t_info.thread;

    Tick latency = 0;

    for (int i = 0; i < width || locked; ++i) {
        numCycles++;
        ppCycles->notify(1);

        if (!curStaticInst || !curStaticInst->isDelayedCommit()) {
            checkForInterrupts();
            checkPcEventQueue();
        }

        // We must have just got suspended by a PC event
        if (_status == Idle) {
            tryCompleteDrain();
            return;
        }

        Fault fault = NoFault;

        TheISA::PCState pcState = thread->pcState();

        bool needToFetch = !isRomMicroPC(pcState.microPC()) &&
                           !curMacroStaticInst;
        if (needToFetch) {
            ifetch_req.taskId(taskId());
            setupFetchRequest(&ifetch_req);
            fault = thread->itb->translateAtomic(&ifetch_req, thread->getTC(),
                                                 BaseTLB::Execute);
        }

        if (fault == NoFault) {
            Tick icache_latency = 0;
            bool icache_access = false;
            dcache_access = false; // assume no dcache access

            if (needToFetch) {
                // This is commented out because the decoder would act like
                // a tiny cache otherwise. It wouldn't be flushed when needed
                // like the I cache. It should be flushed, and when that works
                // this code should be uncommented.
                //Fetch more instruction memory if necessary
                //if (decoder.needMoreBytes())
                //{
                    icache_access = true;
                    Packet ifetch_pkt = Packet(&ifetch_req, MemCmd::ReadReq);
                    ifetch_pkt.dataStatic(&inst);

                    if (fastmem && system->isMemAddr(ifetch_pkt.getAddr()))
                        system->getPhysMem().access(&ifetch_pkt);
                    else
                        icache_latency = icachePort.sendAtomic(&ifetch_pkt);

                    assert(!ifetch_pkt.isError());

                    // ifetch_req is initialized to read the instruction directly
                    // into the CPU object's inst field.
                //}
            }

            preExecute();

            if (curStaticInst) {
                LAInsn *la_insn = dynamic_cast<LAInsn*>(curStaticInst.get());
                if (curStaticInst && la_insn != nullptr) {
                    fault = executeLAInsn(*la_insn, &t_info);
                } else {
                    fault = curStaticInst->execute(&t_info, traceData);
                }

                // keep an instruction count
                if (fault == NoFault) {
                    countInst();
                    ppCommit->notify(std::make_pair(thread, curStaticInst));
                }
                else if (traceData && !DTRACE(ExecFaulting)) {
                    delete traceData;
                    traceData = NULL;
                }

                postExecute();
            }

            // @todo remove me after debugging with legion done
            if (curStaticInst && (!curStaticInst->isMicroop() ||
                        curStaticInst->isFirstMicroop()))
                instCnt++;

            Tick stall_ticks = 0;
            if (simulate_inst_stalls && icache_access)
                stall_ticks += icache_latency;

            if (simulate_data_stalls && dcache_access)
                stall_ticks += dcache_latency;

            if (stall_ticks) {
                // the atomic cpu does its accounting in ticks, so
                // keep counting in ticks but round to the clock
                // period
                latency += divCeil(stall_ticks, clockPeriod()) *
                    clockPeriod();
            }

        }
        if (fault != NoFault || !t_info.stayAtPC)
            advancePC(fault);
    }

    if (tryCompleteDrain())
        return;

    // instruction takes at least one cycle
    if (latency < clockPeriod())
        latency = clockPeriod();

    if (_status != Idle)
        reschedule(tickEvent, curTick() + latency, true);
}

void
AtomicLACoreSimpleCPU::regProbePoints()
{
    BaseCPU::regProbePoints();

    ppCommit = new ProbePointArg<pair<SimpleThread*, const StaticInstPtr>>
                                (getProbeManager(), "Commit");
}

void
AtomicLACoreSimpleCPU::printAddr(Addr a)
{
    dcachePort.printAddr(a);
}

////////////////////////////////////////////////////////////////////////
//
//  AtomicLACoreSimpleCPU Simulation Object
//
AtomicLACoreSimpleCPU *
AtomicLACoreSimpleCPUParams::create()
{
    return new AtomicLACoreSimpleCPU(this);
}

//=========================================================================
// Read/Write to Mem/Scratch interface functions!
//=========================================================================

    //atomic LACore accesses
Fault 
AtomicLACoreSimpleCPU::writeLAMemAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    return writeMem(data, size, addr, 0, NULL);
}

Fault 
AtomicLACoreSimpleCPU::writeLAScratchAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    return scratchpad.writeAtomic(addr, data, size);
}

Fault 
AtomicLACoreSimpleCPU::readLAMemAtomic(Addr addr, uint8_t *data, 
    uint64_t size, ThreadContext *tc)
{
    return readMem(addr, data, size, 0);
}

Fault 
AtomicLACoreSimpleCPU::readLAScratchAtomic(Addr addr, uint8_t *data,
    uint64_t size, ThreadContext *tc)
{
    return scratchpad.readAtomic(addr, data, size);
}


//timing LACore accesses
bool 
AtomicLACoreSimpleCPU::writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size, 
    ThreadContext *tc, uint8_t channel, std::function<void(void)> callback)
{
    panic("AtomicLACoreSimpleCPU::writeLAMemTiming should not be called");
}

bool 
AtomicLACoreSimpleCPU::writeLAScratchTiming(Addr addr, uint8_t *data, 
    uint8_t size, ThreadContext *tc, uint8_t channel,
    std::function<void(void)> callback)
{
    panic("AtomicLACoreSimpleCPU::writeLAScratchTiming should not be called");
}

bool 
AtomicLACoreSimpleCPU::readLAMemTiming(Addr addr, uint8_t size, 
    ThreadContext *tc, uint8_t channel,
    std::function<void(uint8_t*,uint8_t)> callback)
{
    panic("AtomicLACoreSimpleCPU::readLAMemTiming should not be called");
}

bool 
AtomicLACoreSimpleCPU::readLAScratchTiming(Addr addr, uint8_t size, 
    ThreadContext *tc, uint8_t channel,
    std::function<void(uint8_t*,uint8_t)> callback)
{
    panic("AtomicLACoreSimpleCPU::readLAScratchTiming should not be called");
}

//============================================================================
// Data LACore Ops
//============================================================================

Fault 
AtomicLACoreSimpleCPU::laDataEx(LAInsn& insn,
    LACoreSimpleExecContext *xc)
{
    bool is_reduce = (insn.isLADataReduce());
    LAReg * srcA = &(xc->getLAReg(insn.lra()));
    LAReg * srcB = &(xc->getLAReg(insn.lrb()));
    LAReg * srcC = &(xc->getLAReg(insn.lrc()));
    LAReg * dst = &(xc->getLAReg(insn.lrd()));
    LACount count = xc->readIntRegFlat(insn.lrxa());

    //no-ops are fine
    if(count == 0){
        return NoFault;
    }

    //if D is scalar, must be single-out, single-stream
    assert(dst->vector() || count == 1 ||
      (insn.isLADataReduce() && !insn.mst()));

    bool A_dp = srcA->dp();
    bool B_dp = srcB->dp();
    bool C_dp = srcC->dp();
    bool D_dp = dst->dp();

    uint64_t DST_SIZE  = D_dp ? sizeof(double) : sizeof(float);

    uint64_t srcA_bytes = count*(A_dp ? sizeof(double) : sizeof(float));
    uint64_t srcB_bytes = count*(B_dp ? sizeof(double) : sizeof(float));
    uint64_t srcC_bytes = count*(C_dp ? sizeof(double) : sizeof(float));
    uint64_t dst_bytes  = count*DST_SIZE;

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
    if(is_reduce){
        dst_bytes = DST_SIZE;
        dst_count = 1;
        if (insn.mst()) {
            mst_count = Acount;

            assert(!Bcount || !mst_count || (Bcount == mst_count));
            mst_count = (mst_count ? mst_count : Bcount);

            assert(!Ccount || !mst_count || (Ccount == mst_count));
            mst_count = (mst_count ? mst_count : Ccount);

            assert(mst_count && (count % mst_count == 0));
            dst_count = (count/mst_count);
            dst_bytes = dst_count*DST_SIZE;
        }
    }

  //allocate the buffers
  uint8_t * Adata = (uint8_t *)malloc(srcA_bytes);
  uint8_t * Bdata = (uint8_t *)malloc(srcB_bytes);
  uint8_t * Cdata = (uint8_t *)malloc(srcC_bytes);
  uint8_t * Ddata = (uint8_t *)malloc(dst_bytes);

  //read the sources from memory
  mmu.readAtomic(Adata, count, srcA, xc);
  mmu.readAtomic(Bdata, count, srcB, xc);
  mmu.readAtomic(Cdata, count, srcC, xc);

  //used for reduce instructions
  double accum_dp = 0.0;
  float  accum_sp = 0.0;

  for(LACount i=0; i<count; ++i){
    if(D_dp){
      double Aitem = (double)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
      double Bitem = (double)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
      double Citem = (double)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

      //(A+-B)*/C or (A*/B)+-C
      //t2 == 'terms' on top, not top-2 anymore!
      double top   =insn.t2()? Aitem+(insn.su() ? -Bitem  : Bitem)
                              : Aitem*(insn.dv() ? 1/Bitem : Bitem);
      double Ditem =insn.t2()? top*  (insn.dv() ? 1/Citem : Citem)
                              : top+  (insn.su() ? -Citem  : Citem);

      if (is_reduce) {
        //scalar output
        switch(insn.rdct()){
          case LAReduce::Min: accum_dp  = std::min(accum_dp, Ditem); break;
          case LAReduce::Max: accum_dp  = std::max(accum_dp, Ditem); break;
          case LAReduce::Sum: accum_dp += Ditem;                     break;
          default:            assert(false);                         break;
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
      float Aitem = (float)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
      float Bitem = (float)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
      float Citem = (float)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

      //(A+-B)*/C or (A*/B)+-C
      //t2 == 'terms' on top, not top-2 anymore!
      float top   = insn.t2()? Aitem+(insn.su() ? -Bitem  : Bitem)
                              : Aitem*(insn.dv() ? 1/Bitem : Bitem);
      float Ditem = insn.t2()? top*  (insn.dv() ? 1/Citem : Citem)
                              : top+  (insn.su() ? -Citem  : Citem);

      if (is_reduce) {
        //scalar output
        switch(insn.rdct()){
          case LAReduce::Min: accum_sp  = std::min(accum_sp, Ditem); break;
          case LAReduce::Max: accum_sp  = std::max(accum_sp, Ditem); break;
          case LAReduce::Sum: accum_sp += Ditem;                     break;
          default:            assert(false);                         break;
        }
        if(insn.mst() && ((i+1) % mst_count == 0)){
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

  mmu.writeAtomic(Ddata, dst_count, dst, xc);

  free(Adata);
  free(Bdata);
  free(Cdata);
  free(Ddata);

  return NoFault;
}

//============================================================================
// Cfg LACore Ops
//============================================================================

Fault 
AtomicLACoreSimpleCPU::laCfgEx(LAInsn& insn,
    LACoreSimpleExecContext *xc)
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
            double val_dp;
            memcpy((uint8_t *)&val_dp, (uint8_t *)&tmp_dp, sizeof(double));
          dst->scalarDouble(val_dp);
        } else {
            uint32_t tmp_fp = xc->readFloatRegBitsFlat(insn.lrfa());
            float val_fp;
            memcpy((uint8_t *)&val_fp, (uint8_t *)&tmp_fp, sizeof(float));
            dst->scalarFloat(val_fp);
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

    return NoFault;
}

//============================================================================
// Xfer LACore Ops
//============================================================================

Fault 
AtomicLACoreSimpleCPU::laXferEx(LAInsn& insn,
    LACoreSimpleExecContext *xc)
{
    LAReg * dst = &(xc->getLAReg(insn.lrd()));
    LAReg * src = &(xc->getLAReg(insn.lra()));

    LACount   count;
    uint64_t  src_bytes;
    uint64_t  dst_bytes;
    uint8_t * src_data;
    uint8_t * dst_data;
    double *  src_tmp_dp;
    double *  dst_tmp_dp;
    float *   src_tmp_fp;
    float *   dst_tmp_fp;

    if(insn.dat()){
        count = xc->readIntRegFlat(insn.lrxa());
        src_bytes = count*(src->dp() ? sizeof(double) : sizeof(float));
        dst_bytes = count*(dst->dp() ? sizeof(double) : sizeof(float));
        src_data = (uint8_t *)malloc(src_bytes);
        dst_data = (uint8_t *)malloc(dst_bytes);

        mmu.readAtomic(src_data, count, src, xc);
        if(src->dp() && !dst->dp()){
            src_tmp_dp = (double *)src_data;
            dst_tmp_fp = (float *)dst_data;
            for(LACount i=0; i<count; ++i){
                dst_tmp_fp[i] = (float)src_tmp_dp[i];
            }
        } else if(!src->dp() && dst->dp()) {
            src_tmp_fp = (float *)src_data;
            dst_tmp_dp = (double *)dst_data;
            for(LACount i=0; i<count; ++i){
                dst_tmp_dp[i] = (double)src_tmp_fp[i];
            }
        } else {
            memcpy(dst_data, src_data, src_bytes);
        }
        mmu.writeAtomic(dst_data, count, dst, xc);

        free(src_data);
        free(dst_data);
    } else if(insn.get()) {
        xc->setIntRegFlat(insn.lrxa(), xc->getLACsrReg().readAll());
    } else if(insn.clr()) {
        xc->getLACsrReg().readAll(); 
    } else {
        assert(false); 
    }

    return NoFault;
}

//============================================================================
// All LACore Ops
//============================================================================

Fault 
AtomicLACoreSimpleCPU::executeLAInsn(LAInsn& insn,
    LACoreSimpleExecContext *xc)
{
    if (insn.isLAData()) {
        return laDataEx(insn, xc);
    } else if (insn.isLACfg()) {
        return laCfgEx(insn, xc);
    } else if (insn.isLAXfer()) {
        return laXferEx(insn, xc);
    } else {
        //bad instruction
        assert(false);
        return NoFault;
    }
    return NoFault;
}
