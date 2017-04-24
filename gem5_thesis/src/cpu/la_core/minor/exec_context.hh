/*
 * Copyright (c) 2011-2014 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
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
 *          Dave Greene
 *          Nathan Binkert
 *          Andrew Bardsley
 *          Samuel Steffl
 */

/**
 * @file
 *
 *  ExecContext bears the exec_context interface for Minor.
 */

#ifndef __CPU_LA_CORE_MINOR_EXEC_CONTEXT_HH__
#define __CPU_LA_CORE_MINOR_EXEC_CONTEXT_HH__

#include "cpu/base.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/minor/dyn_inst.hh"
#include "cpu/la_core/thread.hh"
#include "debug/MinorExecute.hh"
#include "mem/request.hh"

class MinorLACoreCPU;

namespace Minor
{

/* Forward declaration of Execute */
class Execute;

/** ExecContext bears the exec_context interface for Minor.  This nicely
 *  separates that interface from other classes such as Pipeline, MinorCPU
 *  and DynMinorInst and makes it easier to see what state is accessed by it.
 */
class MinorLACoreExecContext : public ::LACoreExecContext
{
  public:
    MinorLACoreCPU &cpu;

    /** ThreadState object, provides all the architectural state. */
    LACoreThread &thread;

    /** The execute stage so we can peek at its contents. */
    Execute &execute;

    /** Instruction for the benefit of memory operations and for PC */
    MinorDynInstPtr inst;

    MinorLACoreExecContext (
        MinorLACoreCPU &cpu_,
        LACoreThread &thread_, Execute &execute_,
        MinorDynInstPtr inst_) :
        LACoreExecContext(&thread),
        cpu(cpu_),
        thread(thread_),
        execute(execute_),
        inst(inst_)
    {
        DPRINTF(MinorExecute, "ExecContext setting PC: %s\n", inst->pc);
        pcState(inst->pc);
        setPredicate(true);
        thread.setIntReg(TheISA::ZeroReg, 0);
#if THE_ISA == ALPHA_ISA
        thread.setFloatReg(TheISA::ZeroReg, 0.0);
#endif
    }
    virtual ~MinorLACoreExecContext() {}

    Fault
    initiateMemRead(Addr addr, unsigned int size,
                    Request::Flags flags) override;

    Fault
    writeMem(uint8_t *data, unsigned int size, Addr addr,
             Request::Flags flags, uint64_t *res) override;

    IntReg
    readIntRegOperand(const StaticInst *si, int idx) override
    {
        return thread.readIntReg(si->srcRegIdx(idx));
    }

    TheISA::FloatReg
    readFloatRegOperand(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::FP_Reg_Base;
        return thread.readFloatReg(reg_idx);
    }

    TheISA::FloatRegBits
    readFloatRegOperandBits(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::FP_Reg_Base;
        return thread.readFloatRegBits(reg_idx);
    }

    void
    setIntRegOperand(const StaticInst *si, int idx, IntReg val) override
    {
        thread.setIntReg(si->destRegIdx(idx), val);
    }

    void
    setFloatRegOperand(const StaticInst *si, int idx,
        TheISA::FloatReg val) override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::FP_Reg_Base;
        thread.setFloatReg(reg_idx, val);
    }

    void
    setFloatRegOperandBits(const StaticInst *si, int idx,
        TheISA::FloatRegBits val) override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::FP_Reg_Base;
        thread.setFloatRegBits(reg_idx, val);
    }

    bool
    readPredicate() override
    {
        return thread.readPredicate();
    }

    void
    setPredicate(bool val) override;

    TheISA::PCState
    pcState() const override
    {
        return thread.pcState();
    }

    void
    pcState(const TheISA::PCState &val) override
    {
        thread.pcState(val);
    }

    TheISA::MiscReg
    readMiscRegNoEffect(int misc_reg) const
    {
        return thread.readMiscRegNoEffect(misc_reg);
    }

    TheISA::MiscReg
    readMiscReg(int misc_reg) override
    {
        return thread.readMiscReg(misc_reg);
    }

    void
    setMiscReg(int misc_reg, const TheISA::MiscReg &val) override
    {
        thread.setMiscReg(misc_reg, val);
    }

    TheISA::MiscReg
    readMiscRegOperand(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::Misc_Reg_Base;
        return thread.readMiscReg(reg_idx);
    }

    void
    setMiscRegOperand(const StaticInst *si, int idx,
        const TheISA::MiscReg &val) override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::Misc_Reg_Base;
        return thread.setMiscReg(reg_idx, val);
    }

    Fault
    hwrei() override
    {
#if THE_ISA == ALPHA_ISA
        return thread.hwrei();
#else
        return NoFault;
#endif
    }

    bool
    simPalCheck(int palFunc) override
    {
#if THE_ISA == ALPHA_ISA
        return thread.simPalCheck(palFunc);
#else
        return false;
#endif
    }

    void
    syscall(int64_t callnum) override
     {
        if (FullSystem)
            panic("Syscall emulation isn't available in FS mode.\n");

        thread.syscall(callnum);
    }

    ThreadContext *tcBase() override { return thread.getTC(); }

    /* @todo, should make stCondFailures persistent somewhere */
    unsigned int readStCondFailures() const override { return 0; }
    void setStCondFailures(unsigned int st_cond_failures) override {}

    ContextID contextId() { return thread.contextId(); }
    /* ISA-specific (or at least currently ISA singleton) functions */

    /* X86: TLB twiddling */
    void
    demapPage(Addr vaddr, uint64_t asn) override
    {
        thread.getITBPtr()->demapPage(vaddr, asn);
        thread.getDTBPtr()->demapPage(vaddr, asn);
    }

    TheISA::CCReg
    readCCRegOperand(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::CC_Reg_Base;
        return thread.readCCReg(reg_idx);
    }

    void
    setCCRegOperand(const StaticInst *si, int idx, TheISA::CCReg val) override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::CC_Reg_Base;
        thread.setCCReg(reg_idx, val);
    }

    void
    demapInstPage(Addr vaddr, uint64_t asn)
    {
        thread.getITBPtr()->demapPage(vaddr, asn);
    }

    void
    demapDataPage(Addr vaddr, uint64_t asn)
    {
        thread.getDTBPtr()->demapPage(vaddr, asn);
    }

    /* ALPHA/POWER: Effective address storage */
    void setEA(Addr ea) override
    {
        inst->ea = ea;
    }

    BaseCPU *getCpuPtr();

    /* POWER: Effective address storage */
    Addr getEA() const override
    {
        return inst->ea;
    }

    /* MIPS: other thread register reading/writing */
    uint64_t
    readRegOtherThread(int idx, ThreadID tid = InvalidThreadID);

    void
    setRegOtherThread(int idx, const TheISA::MiscReg &val,
        ThreadID tid = InvalidThreadID);

  public:
    // monitor/mwait funtions
    void armMonitor(Addr address) override;

    bool mwait(PacketPtr pkt) override;

    void mwaitAtomic(ThreadContext *tc) override;

    AddressMonitor *getAddrMonitor() override;



//=========================================================================
// LACore Register Accesses
//=========================================================================

    uint64_t readIntRegFlat(int idx) override
    { 
        return thread.readIntRegFlat(idx);
    }
    void setIntRegFlat(int idx, uint64_t val) override
    {
        thread.setIntRegFlat(idx, val);
    }
    FloatRegBits readFloatRegBitsFlat(int idx) override
    {
        return thread.readFloatRegBitsFlat(idx);
    }
    void setFloatRegBitsFlat(int idx, FloatRegBits val) override
    {
        thread.setFloatRegBitsFlat(idx, val);
    }

    LAReg& getLAReg(unsigned idx) override
    {
        return thread.laRegs[idx];
    }

    LACsrReg& getLACsrReg() override
    {
        return thread.laCsrReg;
    }

//=========================================================================
// LACore Reading/Writing to Scratchpad/memory
//=========================================================================

    //atomic LACore accesses
    Fault readLAMemAtomic(Addr addr, uint8_t *data, uint64_t size) override;
    Fault writeLAMemAtomic(Addr addr, uint8_t *data, uint64_t size) override;
    Fault readLAScratchAtomic(Addr addr, uint8_t *data, uint64_t size) override;
    Fault writeLAScratchAtomic(Addr addr, uint8_t *data, 
        uint64_t size) override;
    
    //timing LACore accesses
    bool writeLAMemTiming(Addr addr, uint8_t *data, uint8_t size,
        uint8_t channel, std::function<void(void)> callback) override;
    bool writeLAScratchTiming(Addr addr, uint8_t *data, uint8_t size,
        uint8_t channel, std::function<void(void)> callback) override;
    bool readLAMemTiming(Addr addr, uint8_t size, uint8_t channel,
        std::function<void(uint8_t*,uint8_t)> callback) override;
    bool readLAScratchTiming(Addr addr, uint8_t size, uint8_t channel,
        std::function<void(uint8_t*,uint8_t)> callback) override;
};

}

#endif /* __CPU_LA_CORE_MINOR_EXEC_CONTEXT_HH__ */
