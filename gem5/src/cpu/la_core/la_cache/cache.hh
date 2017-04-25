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

#ifndef __CPU_LA_CORE_LA_CACHE_HH__
#define __CPU_LA_CORE_LA_CACHE_HH__

#include <cstdint>
#include <string>
#include <vector>

#include "cpu/la_core/packet.hh"
#include "mem/cache/cache.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/LACoreLACache.hh"

// the L1 execCache attached to the execution units in the LaCore
// it has N ports facing the LACore and M L1 bank slices, so its a multi-port
// multi-banked L1 cache
class LACoreLACache : public MemObject
{
  public:
    //the single port facing the LACore that multiplexes requests among banks
    class LACoreSlavePort : public SlavePort
    {
      public:
        LACoreSlavePort(const std::string& name, LACoreLACache& la_cache);
        ~LACoreSlavePort();

      protected:
        Tick recvAtomic(PacketPtr pkt) override;
        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr pkt) override;
        void recvRespRetry() override;
        AddrRangeList getAddrRanges() const override;

      private:
        LACoreLACache& laCache;
    };

    //the port facing the XBar that connects LACore ports to the LACache banks
    class XbarPort : public MasterPort
    {
      public:
        XbarPort(const std::string& name, LACoreLACache& la_cache);
        ~XbarPort();

        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;

      private:
        LACoreLACache& laCache;
    };

  public:
    LACoreLACache(const LACoreLACacheParams *p);
    ~LACoreLACache();

    BaseMasterPort& getMasterPort(const std::string& if_name,
                                PortID idx = InvalidPortID) override;
    BaseSlavePort& getSlavePort(const std::string& if_name,
                                PortID idx = InvalidPortID) override;

    const uint64_t numPorts;
    LACoreSlavePort laCoreSlavePort;
    std::vector<XbarPort> xbarPorts;
};


#endif // __CPU_LA_CORE_LA_CACHE_HH__
