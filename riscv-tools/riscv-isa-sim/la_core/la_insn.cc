//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "la_insn.h"

#include <cassert>

#include "la_core.h"
#include "processor.h"

namespace la_core {

reg_t la_data_ex(processor_t* p, insn_t insn, reg_t pc)
{
  LACore * lac = static_cast<LACore *>(p->get_extension());
  LAInsn la_insn(insn.bits());
  assert(la_insn.is_la_data());
  lac->la_data_ex(la_insn);
  return pc + 4;
}

reg_t la_cfg_ex(processor_t* p, insn_t insn, reg_t pc)
{
  LACore * lac = static_cast<LACore *>(p->get_extension());
  LAInsn la_insn(insn.bits());
  assert(la_insn.is_la_cfg());
  lac->la_cfg_ex(la_insn);
  return pc + 4;
}

reg_t la_xfer_ex(processor_t* p, insn_t insn, reg_t pc)
{

  LACore * lac = static_cast<LACore *>(p->get_extension());
  LAInsn la_insn(insn.bits());
  assert(la_insn.is_la_xfer());
  lac->la_xfer_ex(la_insn);
  return pc + 4;
}

} //namespace la_core
