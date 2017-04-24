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

#ifndef __ARCH_RISCV_EXTENSION_REGISTERS_HH__
#define __ARCH_RISCV_EXTENSION_REGISTERS_HH__

#include "arch/riscv/registers.hh"

namespace RiscvISA {

//=============================================================================
// RISC-V LACore Registers
//=============================================================================

const int NumLARegs = 8;
const int NumLACsrRegs = 1;

// NOTE: the LACore Extension uses up to 3 src regs so we are lucky -- don't
//       have to mess with MaxInstSrcRegs or MaxInstDestRegs here

// These help enumerate all the registers for dependence tracking.
const int LA_Reg_Base = Max_Base_Reg_index;
const int LA_Csr_Reg_Base = LA_Reg_Base + NumLARegs;
const int Max_Extension_Reg_Index = LA_Csr_Reg_Base + NumLACsrRegs;

const int Max_Reg_Index = Max_Extension_Reg_Index;


//=============================================================================
// RISC-V LACore Register Names
//=============================================================================

const char * const IntRegNamesNumeric[] = 
{
    "x0",   "x1",   "x2",   "x3",   "x4",   "x5",   "x6",   "x7",
    "x8",   "x9",   "x10",  "x11",  "x12",  "x13",  "x14",  "x15",
    "x16",  "x17",  "x18",  "x19",  "x20",  "x21",  "x22",  "x23",
    "x24",  "x25",  "x26",  "x27",  "x28",  "x29",  "x30",  "x31"
};

const char * const FloatRegNamesNumeric[] = 
{
    "f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
    "f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
    "f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
    "f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31"
};


const char * const LARegNamesNumeric[] = 
{
  "lr0", "lr1", "lr2", "lr3", "lr4", "lr5", "lr6", "lr7"
};

const char * const LACsrRegNamesNumeric[] = 
{
    "lcsr"
};

} // namespace RiscvISA

#endif // __ARCH_RISCV_EXTENSION_REGISTERS_HH__
