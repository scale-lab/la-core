//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_LA_INSN__
#define __CPU_LA_CORE_LA_INSN__

#include "arch/registers.hh"
#include "cpu/static_inst.hh"

//=============================================================================
// LACore Instruction Constants and Enums
//=============================================================================

enum class LAReduce { 
  Min = 0,
  Max = 1,
  Sum = 2
};

enum class LALoc {
  Reg = 0,
  Mem = 1,
  Sch = 2
};

//=============================================================================
// LACore Instruction
//=============================================================================

class LAInsn : public StaticInst
{
public:
  LAInsn(const char *_mnemonic, ExtMachInst _machInst, OpClass __opClass) :
    StaticInst(_mnemonic, _machInst, __opClass) {}
  ~LAInsn() {}

  virtual bool isLAData() const = 0;
  virtual bool isLADataReduce() const = 0;
  virtual bool isLACfg() const = 0;
  virtual bool isLAXfer() const = 0;

  virtual RegIndex lrxc() const = 0;
  virtual RegIndex lrxb() const = 0;
  virtual RegIndex lrxa() const = 0;
  virtual RegIndex lrfa() const = 0;

  virtual RegIndex lrd() const = 0;
  virtual RegIndex lrc() const = 0;
  virtual RegIndex lrb() const = 0;
  virtual RegIndex lra() const = 0;

  virtual LALoc lrdloc() const = 0;
  virtual bool lrdHasMemRef() const = 0;

  virtual bool t2() const = 0;
  virtual bool su() const = 0;
  virtual bool dv() const = 0;
  virtual bool alt() const = 0;
  virtual bool vec() const = 0;
  virtual bool dp() const = 0;
  virtual bool dat() const = 0;
  virtual bool get() const = 0;
  virtual bool clr() const = 0;

  virtual LAReduce rdct() const = 0;
  virtual bool tns() const = 0;
  virtual bool spv() const = 0;

  virtual bool mst() const = 0;

};


#endif // __CPU_LA_CORE_LA_INSN__
