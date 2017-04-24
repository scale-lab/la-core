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

#ifndef __CPU_LA_CORE_THREAD_HH__
#define __CPU_LA_CORE_THREAD_HH__

#include "arch/isa.hh"
#include "arch/isa_traits.hh"
#include "arch/registers.hh"
#include "arch/tlb.hh"
#include "arch/types.hh"
#include "base/types.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"
#include "cpu/simple_thread.hh"
#include "config/the_isa.hh"
#include "sim/process.hh"
#include "sim/system.hh"

class LACoreThread : public SimpleThread
{
  public:
    LACsrReg laCsrReg;
    LAReg laRegs[TheISA::NumLARegs];

//=============================================================================
// constructor/destructor
//=============================================================================

  public:
    // FS
    LACoreThread(BaseCPU *_cpu, int _thread_num, System *_system,
        TheISA::TLB *_itb, TheISA::TLB *_dtb, TheISA::ISA *_isa,
        bool use_kernel_stats = true) :
        SimpleThread(_cpu, _thread_num, _system, _itb, _dtb, _isa, 
            use_kernel_stats) 
        {}

    // SE
    LACoreThread(BaseCPU *_cpu, int _thread_num, System *_system,
        Process *_process, TheISA::TLB *_itb, TheISA::TLB *_dtb,
        TheISA::ISA *_isa) :
        SimpleThread(_cpu, _thread_num, _system, _process, _itb, _dtb, _isa) {}

    ~LACoreThread() {}
};


#endif // __CPU_LA_CORE_THREAD_HH__
