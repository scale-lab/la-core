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

#ifndef __CPU_LA_CORE_SCRATCHPAD_SCRATCHPAD_HH__
#define __CPU_LA_CORE_SCRATCHPAD_SCRATCHPAD_HH__

#include <cstdint>
#include <string>
#include <vector>

#include "base/types.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/qport.hh"
#include "params/LACoreScratchpad.hh"
#include "sim/ticked_object.hh"

// Single port, parameterizable scratchpad memory for the LaCore. it consists 
// of a multi-bank physical scratchpad that can perform gather/scatters to 
// each bank every cycle. banks are 8-byte interleaved
class LACoreScratchpad : public MemObject
{
  public:
    class LACoreScratchpadPort : public QueuedSlavePort
    {
      public:
        LACoreScratchpadPort(const std::string& name,
            LACoreScratchpad& scratchpad);

      protected:
        Tick recvAtomic(PacketPtr pkt) override;
        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr pkt) override;
        AddrRangeList getAddrRanges() const override;

      private:
        RespPacketQueue queue;
        LACoreScratchpad& scratchpad;
    };

  public:
    LACoreScratchpad(const LACoreScratchpadParams *p);
    ~LACoreScratchpad();

    BaseSlavePort& getSlavePort(const std::string& if_name,
                                PortID idx = InvalidPortID) override;

    // called by the LACoreScratchpadPort on a received packet
    bool handleTimingReq(PacketPtr pkt, LACoreScratchpadPort *port);

  private:
    //python configuration
    const uint64_t size;
    const uint64_t lineSize;
    const uint64_t numPorts;
    const uint64_t accessLatency;

    //how many bytes per line are distributed to each bank
    uint64_t numBanks;
    uint64_t bytesPerBankAccess;

    // the slave port to access the scratchpad
    std::vector<LACoreScratchpadPort *> ports;

    //the physical scratchpad storage
    uint8_t * data;
};

#endif //__CPU_LA_CORE_SCRATCHPAD_SCRATCHPAD_HH__
