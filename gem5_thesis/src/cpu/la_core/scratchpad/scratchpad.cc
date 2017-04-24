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
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
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
 * Authors: Samuel Steffl
 */

#include "cpu/la_core/scratchpad/scratchpad.hh"

#include <cassert>
#include <cstdint>
#include <vector>
#include <string>

#include "base/misc.hh"
#include "debug/LACoreScratchpad.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/qport.hh"
#include "params/LACoreScratchpad.hh"

//=============================================================================
// LACoreScratchpad::LACoreScratchpadPort
//=============================================================================

LACoreScratchpad::LACoreScratchpadPort::LACoreScratchpadPort(
    const std::string &name, LACoreScratchpad& scratchpad) : 
    QueuedSlavePort(name, &scratchpad, queue), queue(scratchpad, *this),
    scratchpad(scratchpad)
{
}

Tick 
LACoreScratchpad::LACoreScratchpadPort::recvAtomic(PacketPtr pkt)
{
    panic("LACoreScratchpadPort::recvAtomic called\n"); 
}

void 
LACoreScratchpad::LACoreScratchpadPort::recvFunctional(PacketPtr pkt)
{
    panic("LACoreScratchpadPort::recvFunctional called\n"); 
}

bool 
LACoreScratchpad::LACoreScratchpadPort::recvTimingReq(PacketPtr pkt)
{
    assert(pkt->isRead() || pkt->isWrite());
    assert(pkt->isRequest());
    assert(pkt->needsResponse());

    return scratchpad.handleTimingReq(pkt, this);
}

AddrRangeList 
LACoreScratchpad::LACoreScratchpadPort::getAddrRanges() const
{
    panic("LACoreScratchpadPort::getAddrRanges called\n"); 
}


//=============================================================================
// LACoreScratchpad constructor/destructor
//=============================================================================

LACoreScratchpad::LACoreScratchpad(const LACoreScratchpadParams* p) :
    MemObject(p), size(p->size), lineSize(p->lineSize),
    numPorts(p->numPorts), accessLatency(p->accessLatency)
{
    assert(size % lineSize == 0);
    assert(lineSize % sizeof(float) == 0);

    //4-byte word is smallest addressable unit!
    numBanks = lineSize / sizeof(float);
    bytesPerBankAccess = lineSize / numBanks;
    data = new uint8_t[size];

    for(uint8_t i=0; i<numPorts; ++i){
        ports.push_back(new LACoreScratchpadPort(name() + ".port", *this));
    }
}

LACoreScratchpad::~LACoreScratchpad()
{
    delete data;
}

//=============================================================================
// LACoreScratchpad memory/port functions
//=============================================================================

bool 
LACoreScratchpad::handleTimingReq(PacketPtr pkt, LACoreScratchpadPort *port)
{
    //need to make sure all accesses happen within a single line
    uint64_t start_addr = pkt->getAddr();
    uint64_t end_addr = pkt->getAddr() + pkt->getSize() -1;
    uint64_t start_line_addr = start_addr - (start_addr % lineSize);
    uint64_t end_line_addr = end_addr - (end_addr % lineSize);
    assert(start_line_addr == end_line_addr);

    //need to make sure we are accessing full data from accessed banks
    assert(start_addr % bytesPerBankAccess == 0);
    assert((end_addr+1) % bytesPerBankAccess == 0);

    if(pkt->isRead()){
        memcpy(pkt->getPtr<uint8_t>(), data+start_addr, pkt->getSize());
        DPRINTF(LACoreScratchpad,
            "LACoreScratchpad read %u bytes from addr %#u\n",
            pkt->getSize(), pkt->getAddr());
    } else {
        memcpy(data+start_addr, pkt->getPtr<uint8_t>(), pkt->getSize());
        DPRINTF(LACoreScratchpad,
            "LACoreScratchpad wrote %u bytes to addr %#u\n",
            pkt->getSize(), pkt->getAddr());
    }

    pkt->makeTimingResponse();
    pkt->headerDelay = 0;
    pkt->payloadDelay = 0;
    port->schedTimingResp(pkt,
        curTick() + cyclesToTicks(Cycles(accessLatency)), true);

    return true;
}

BaseSlavePort &
LACoreScratchpad::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name != "port") {
        return MemObject::getSlavePort(if_name, idx);
    } else {
        return *ports[idx];
    }
}


//=============================================================================
// LACoreScratchpadParams
//=============================================================================

LACoreScratchpad *
LACoreScratchpadParams::create()
{
    return new LACoreScratchpad(this);
}
