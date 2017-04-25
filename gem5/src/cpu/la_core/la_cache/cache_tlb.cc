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

#include "cpu/la_core/la_cache/cache_tlb.hh"

#include <cassert>
#include <cstdint>

#include "arch/generic/tlb.hh"
#include "base/misc.hh"
#include "base/types.hh"
#include "cpu/la_core/la_cache/translation.hh"
#include "cpu/thread_context.hh"
#include "debug/LACoreLACacheTLB.hh"
#include "mem/request.hh"
#include "params/LACoreLACacheTLB.hh"
#include "sim/faults.hh"
#include "sim/process.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LACoreLACacheTLB::LACoreLACacheTLB(const LACoreLACacheTLBParams *p) :
    BaseTLB(p), latency(p->latency)
{
}

LACoreLACacheTLB::~LACoreLACacheTLB()
{
}

//=============================================================================
// overrides (not implemented)
//=============================================================================

void 
LACoreLACacheTLB::demapPage(Addr vaddr, uint64_t asn)
{
    panic("LACoreLACacheTLB::demapPage not implemented!\n");
}

void 
LACoreLACacheTLB::flushAll()
{
    panic("LACoreLACacheTLB::flushAll not implemented!\n");
}

void
LACoreLACacheTLB::takeOverFrom(BaseTLB *otlb)
{
    panic("LACoreLACacheTLB::takeOverFrom not implemented!\n");
}

//=============================================================================
// actual translation interface
//=============================================================================

// for now, do fixed-latency lookups, investigate a more accurate tlb model
// in the future
Fault
LACoreLACacheTLB::translateAtomic(RequestPtr req, ThreadContext *tc)
{
    if (FullSystem) {
        panic("Generic translation shouldn't be used in full system mode.\n");
    }
    Fault fault = tc->getProcessPtr()->pTable->translate(req);
    DPRINTF(LACoreLACacheTLB,
        "LACoreLACacheTLB translated vaddr(%#X) to paddr(%#X)\n",
        req->getVaddr(), req->getPaddr());
    return fault;
}

void
LACoreLACacheTLB::translateTiming(RequestPtr req, ThreadContext *tc,
    LACoreLACacheTranslation *translation)
{
    translation->finish(translateAtomic(req, tc), latency);
}

//=============================================================================
// LACoreLACacheTLBParams factory
//=============================================================================

LACoreLACacheTLB *
LACoreLACacheTLBParams::create()
{
    return new LACoreLACacheTLB(this);
}
