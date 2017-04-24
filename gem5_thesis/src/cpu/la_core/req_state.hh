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

#ifndef __CPU_LA_CORE_REQ_STATE_HH__
#define __CPU_LA_CORE_REQ_STATE_HH__

#include <cassert>
#include <cstdint>
#include <functional>

#include "mem/packet.hh"
#include "mem/request.hh"

//=========================================================================
// LAMemReqState
//=========================================================================

// basic state for a request to either the scratchpad or exec-cache. uniquely
// identifies any read/write with a unique reqId. A callback must be executed
// on the completion of the request
class LAMemReqState
{
  public:
    LAMemReqState(uint64_t req_id) : 
        reqId(req_id), matched(false), pkt(nullptr) 
    {}
    virtual ~LAMemReqState() {
        if (pkt != nullptr) {
            assert(pkt->req != nullptr);
            delete pkt->req;
            delete pkt;
        }
    }

    //read/write have different callback type-definitions. so abstract it here
    virtual void executeCallback() = 0;

    uint64_t getReqId() { return reqId; }
    bool isMatched() { return matched; }
    void setPacket(PacketPtr _pkt) { 
        assert(!matched);
        matched = true;
        pkt = _pkt;
    }

  protected:
    uint64_t reqId;
    bool matched;
    PacketPtr pkt;
};

//=========================================================================
// LAMemReadReqState
//=========================================================================

class LAMemReadReqState : public LAMemReqState
{
  public:
    LAMemReadReqState(uint64_t req_id, 
        std::function<void(uint8_t*data, uint8_t size)> callback) :
        LAMemReqState(req_id), callback(callback) {}
    ~LAMemReadReqState() {}

    void executeCallback() override { 
        assert(matched);
        callback(pkt->getPtr<uint8_t>(), pkt->getSize());
    }

  private:
    std::function<void(uint8_t*data, uint8_t size)> callback;
};

//=========================================================================
// LAMemWriteReqState
//=========================================================================

class LAMemWriteReqState : public LAMemReqState
{
  public:
    LAMemWriteReqState(uint64_t req_id, std::function<void(void)> callback) :
        LAMemReqState(req_id), callback(callback) {}
    ~LAMemWriteReqState() {}

    void executeCallback() override { 
        assert(matched);
        callback(); 
    }

  private:
    std::function<void(void)> callback;
};

#endif // __CPU_LA_CORE_REQ_STATE_HH__
