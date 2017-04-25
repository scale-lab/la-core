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

#include "cpu/la_core/la_cache/cache.hh"

#include <cstdint>
#include <string>
#include <vector>

#include "base/misc.hh"
#include "cpu/la_core/packet.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/LACoreLACache.hh"


//=============================================================================
// LACoreLACache::LACoreSlavePort
//=============================================================================

LACoreLACache::LACoreSlavePort::LACoreSlavePort(
    const std::string &name, LACoreLACache& la_cache) : 
    SlavePort(name, &la_cache), laCache(la_cache)
{
}

LACoreLACache::LACoreSlavePort::~LACoreSlavePort()
{
}

Tick 
LACoreLACache::LACoreSlavePort::recvAtomic(PacketPtr pkt)
{
    panic("LACoreSlavePort::recvAtomic called\n"); 
}

void 
LACoreLACache::LACoreSlavePort::recvFunctional(PacketPtr pkt)
{
    panic("LACoreSlavePort::recvFunctional called\n"); 
}

bool 
LACoreLACache::LACoreSlavePort::recvTimingReq(PacketPtr pkt)
{
    LACorePacketPtr la_pkt = dynamic_cast<LACorePacketPtr>(pkt);

    assert(la_pkt != nullptr);
    assert(la_pkt->isRead() || la_pkt->isWrite());
    assert(la_pkt->isRequest());
    assert(la_pkt->needsResponse());
    assert(la_pkt->channel < laCache.numPorts);

    return laCache.xbarPorts[la_pkt->channel].sendTimingReq(pkt);
}

void
LACoreLACache::LACoreSlavePort::recvRespRetry()
{
    //NOTE: not sure if we should just panic here
    for (auto& xbarPort : laCache.xbarPorts) {
        xbarPort.sendRetryResp();
    }
}

AddrRangeList 
LACoreLACache::LACoreSlavePort::getAddrRanges() const
{
    panic("LACoreSlavePort::getAddrRanges called\n"); 
}

//=============================================================================
// LACoreLACache::XbarPort
//=============================================================================

LACoreLACache::XbarPort::XbarPort(
    const std::string& name, LACoreLACache& la_cache) :
    MasterPort(name, &la_cache), laCache(la_cache)
{
}

LACoreLACache::XbarPort::~XbarPort()
{
}

bool
LACoreLACache::XbarPort::recvTimingResp(PacketPtr pkt)
{
    return laCache.laCoreSlavePort.sendTimingResp(pkt);
}

void
LACoreLACache::XbarPort::recvReqRetry()
{
    //TODO: figure out how to make this channelized? eh, you can't oh well.
    laCache.laCoreSlavePort.sendRetryReq();
}
   

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreLACache::LACoreLACache(const LACoreLACacheParams *p) :
    MemObject(p), numPorts(p->numPorts),
    laCoreSlavePort(name() + ".la_core_side", *this)
{
    for(uint8_t i=0; i<numPorts; ++i){
        xbarPorts.push_back(XbarPort(
            (std::string(name() + ".xbar_side_") + std::to_string(i)),
            *this));
    }
}

LACoreLACache::~LACoreLACache()
{
}

//=============================================================================
// Port Functions
//=============================================================================

BaseMasterPort &
LACoreLACache::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "xbar_side")
        return xbarPorts[idx];
    else
        return MemObject::getMasterPort(if_name, idx);
}

BaseSlavePort &
LACoreLACache::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "la_core_side")
        return laCoreSlavePort;
    else
        return MemObject::getSlavePort(if_name, idx);
}

//=============================================================================
// LACoreLACacheParams factory
//=============================================================================

LACoreLACache *
LACoreLACacheParams::create()
{
    return new LACoreLACache(this);
}
