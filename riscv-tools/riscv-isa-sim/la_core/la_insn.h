//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __LA_INSN__
#define __LA_INSN__

#include <cassert>
#include <string>
#include <vector>

#include "decode.h"
#include "disasm.h"
#include "encoding.h"
#include "processor.h"

namespace la_core {

//=============================================================================
// RISC-V Scientific Instruction Constants and Enums
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
// RISC-V LACore Instruction
//=============================================================================

class LAInsn
{
public:
  LAInsn(uint32_t bits) : b(bits) {}
  ~LAInsn() {}
  
  uint32_t bits() { return b; }

  bool is_la_opcode(){ return opcode() == 0x0b; }
  bool is_la_data(){ return is_la_opcode() && (lop() < 0x02); }
  bool is_la_cfg(){  return is_la_opcode() && (lop() == 0x02); }
  bool is_la_xfer(){ return is_la_opcode() && (lop() == 0x03); }

  uint32_t opcode() { return x(0, 7); }
  uint32_t lop() {    return x(7, 2); }
  uint32_t lfunc3() { return x(9, 3); }
  uint32_t lfunc2() { return x(15, 2); }

  uint32_t lrxc(){    return x(17, 5); }
  uint32_t lrxb(){    return x(22, 5); }
  uint32_t lrxa(){    return x(27, 5); }
  uint32_t lrfa(){    return x(27, 5); }

  uint32_t lrd(){     return x(12, 3); }
  uint32_t lrc(){     return x(18, 3); }
  uint32_t lrb(){     return x(21, 3); }
  uint32_t lra(){     return x(24, 3); }

  LALoc lrdloc(){     return (LALoc)(x(15, 2)); }
  uint32_t lrd_has_mem_ref() { return is_la_cfg() && 
                                ((lfunc3() == 0x0) || (lfunc3() == 0x3) || 
                                 (lfunc3() == 0x4) || (lfunc3() == 0x7));  }

  uint32_t t2(){      return x(9,  1); }
  uint32_t su(){      return x(10, 1); }
  uint32_t dv() {     return x(11, 1); }
  uint32_t alt() {    return x(9,  1); }
  uint32_t vec() {    return x(10, 1); }
  uint32_t dp() {     return x(11, 1); }
  uint32_t dat() {    return x(9,  1); }
  uint32_t get() {    return x(10, 1); }
  uint32_t clr() {    return x(11, 1); }

  LAReduce rdct(){    return (LAReduce)(x(15, 2)); }
  uint32_t tns(){     return x(15, 1); }
  uint32_t spv() {    return x(16, 1); }

  uint32_t mst() {    return x(17, 1); }

  uint32_t lzero9(){  return x(15, 9); }

private:
  uint32_t b;
  uint32_t x(int lo, int len) {
    return (b >> lo) & ((uint32_t(1) << len)-1);
  }
};

/*
typedef union la_insn_union
{
  insn_t insn;
  LAInsn la_insn;
}
la_insn_union_t;
*/

//=============================================================================
// RISC-V LACore Instruction Disassembly
//=============================================================================

extern const char* xpr_name[];
extern const char* fpr_name[];

const ::std::vector<::std::string> la_core_reg_name = {
  "lr0", "lr1", "lr2", "lr3", "lr4", "lr5", "lr6", "lr7"
};

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    return la_core_reg_name[la_insn.lra()];
  }
} lra;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    return la_core_reg_name[la_insn.lrb()];
  }
} lrb;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    //la_insn_union_t u;
    //u.insn = insn;
    return la_core_reg_name[la_insn.lrc()];
  }
} lrc;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    //la_insn_union_t u;
    //u.insn = insn;
    ::std::string pre ("");
    ::std::string post ("");
    if( la_insn.lrd_has_mem_ref() ){
      if(la_insn.lrdloc() == LALoc::Mem) { pre = "("; post = ")"; }
      if(la_insn.lrdloc() == LALoc::Sch) { pre = "{"; post = "}"; }
    }
    return pre + la_core_reg_name[la_insn.lrd()] + post;
  }
} lrd;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    //la_insn_union_t u;
    //u.insn = insn;
    return ::xpr_name[la_insn.lrxa()];
  }
} lrxa;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    //la_insn_union_t u;
    //u.insn = insn;
    return ::xpr_name[la_insn.lrxb()];
  }
} lrxb;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    //la_insn_union_t u;
    //u.insn = insn;
    return ::xpr_name[la_insn.lrxc()];
  }
} lrxc;

struct : public arg_t {
  ::std::string to_string(insn_t insn) const {
    LAInsn la_insn((uint32_t)insn.bits());
    //la_insn_union_t u;
    //u.insn = insn;
    return ::fpr_name[la_insn.lrfa()];
  }
} lrfa;

//=============================================================================
// RISC-V LACore Instruction Description
//=============================================================================

// insn_desc_t contains info for both get_instructions() and get_disasms()
typedef struct la_insn_desc
{
  const char *name;
  insn_bits_t match;
  insn_bits_t mask;
  insn_func_t func;
  ::std::vector<const arg_t*> args;
}
la_insn_desc_t;

reg_t la_data_ex(processor_t* p, insn_t insn, reg_t pc);
reg_t la_cfg_ex(processor_t* p, insn_t insn, reg_t pc);
reg_t la_xfer_ex(processor_t* p, insn_t insn, reg_t pc);

//=============================================================================
// RISC-V LACore Instruction Descriptions
//=============================================================================

const ::std::vector<la_insn_desc_t> la_insn_descs = {

//DATA OPS
{"ldrmabn",  MATCH_LDRMABN,  MASK_LDRMABN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmabx",  MATCH_LDRMABX,  MASK_LDRMABX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmabs",  MATCH_LDRMABS,  MASK_LDRMABS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmatn",  MATCH_LDRMATN,  MASK_LDRMATN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmatx",  MATCH_LDRMATX,  MASK_LDRMATX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmats",  MATCH_LDRMATS,  MASK_LDRMATS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmsbn",  MATCH_LDRMSBN,  MASK_LDRMSBN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmsbx",  MATCH_LDRMSBX,  MASK_LDRMSBX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmsbs",  MATCH_LDRMSBS,  MASK_LDRMSBS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmstn",  MATCH_LDRMSTN,  MASK_LDRMSTN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmstx",  MATCH_LDRMSTX,  MASK_LDRMSTX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmsts",  MATCH_LDRMSTS,  MASK_LDRMSTS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdabn",  MATCH_LDRDABN,  MASK_LDRDABN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdabx",  MATCH_LDRDABX,  MASK_LDRDABX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdabs",  MATCH_LDRDABS,  MASK_LDRDABS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdatn",  MATCH_LDRDATN,  MASK_LDRDATN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdatx",  MATCH_LDRDATX,  MASK_LDRDATX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdats",  MATCH_LDRDATS,  MASK_LDRDATS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdsbn",  MATCH_LDRDSBN,  MASK_LDRDSBN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdsbx",  MATCH_LDRDSBX,  MASK_LDRDSBX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdsbs",  MATCH_LDRDSBS,  MASK_LDRDSBS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdstn",  MATCH_LDRDSTN,  MASK_LDRDSTN,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdstx",  MATCH_LDRDSTX,  MASK_LDRDSTX,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrdsts",  MATCH_LDRDSTS,  MASK_LDRDSTS,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmabn", MATCH_LDRMMABN, MASK_LDRMMABN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmabx", MATCH_LDRMMABX, MASK_LDRMMABX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmabs", MATCH_LDRMMABS, MASK_LDRMMABS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmatn", MATCH_LDRMMATN, MASK_LDRMMATN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmatx", MATCH_LDRMMATX, MASK_LDRMMATX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmats", MATCH_LDRMMATS, MASK_LDRMMATS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmsbn", MATCH_LDRMMSBN, MASK_LDRMMSBN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmsbx", MATCH_LDRMMSBX, MASK_LDRMMSBX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmsbs", MATCH_LDRMMSBS, MASK_LDRMMSBS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmstn", MATCH_LDRMMSTN, MASK_LDRMMSTN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmstx", MATCH_LDRMMSTX, MASK_LDRMMSTX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmmsts", MATCH_LDRMMSTS, MASK_LDRMMSTS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdabn", MATCH_LDRMDABN, MASK_LDRMDABN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdabx", MATCH_LDRMDABX, MASK_LDRMDABX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdabs", MATCH_LDRMDABS, MASK_LDRMDABS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdatn", MATCH_LDRMDATN, MASK_LDRMDATN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdatx", MATCH_LDRMDATX, MASK_LDRMDATX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdats", MATCH_LDRMDATS, MASK_LDRMDATS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdsbn", MATCH_LDRMDSBN, MASK_LDRMDSBN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdsbx", MATCH_LDRMDSBX, MASK_LDRMDSBX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdsbs", MATCH_LDRMDSBS, MASK_LDRMDSBS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdstn", MATCH_LDRMDSTN, MASK_LDRMDSTN, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdstx", MATCH_LDRMDSTX, MASK_LDRMDSTX, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldrmdsts", MATCH_LDRMDSTS, MASK_LDRMDSTS, la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmab",   MATCH_LDVMAB,   MASK_LDVMAB,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmat",   MATCH_LDVMAT,   MASK_LDVMAT,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmsb",   MATCH_LDVMSB,   MASK_LDVMSB,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmst",   MATCH_LDVMST,   MASK_LDVMST,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvdab",   MATCH_LDVDAB,   MASK_LDVDAB,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvdat",   MATCH_LDVDAT,   MASK_LDVDAT,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvdsb",   MATCH_LDVDSB,   MASK_LDVDSB,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvdst",   MATCH_LDVDST,   MASK_LDVDST,   la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmmab",  MATCH_LDVMMAB,  MASK_LDVMMAB,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmmat",  MATCH_LDVMMAT,  MASK_LDVMMAT,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmmsb",  MATCH_LDVMMSB,  MASK_LDVMMSB,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmmst",  MATCH_LDVMMST,  MASK_LDVMMST,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmdab",  MATCH_LDVMDAB,  MASK_LDVMDAB,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmdat",  MATCH_LDVMDAT,  MASK_LDVMDAT,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmdsb",  MATCH_LDVMDSB,  MASK_LDVMDSB,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },
{"ldvmdst",  MATCH_LDVMDST,  MASK_LDVMDST,  la_data_ex, {&lrd, &lra, &lrb, &lrc, &lrxa} },

//CFG OPS
{"lcfgsxs",   MATCH_LCFGSXS,   MASK_LCFGSXS,   la_cfg_ex, {&lrd, &lrxa} },
{"lcfgsxd",   MATCH_LCFGSXD,   MASK_LCFGSXD,   la_cfg_ex, {&lrd, &lrxa} },
{"lcfgsfs",   MATCH_LCFGSFS,   MASK_LCFGSFS,   la_cfg_ex, {&lrd, &lrfa} },
{"lcfgsfd",   MATCH_LCFGSFD,   MASK_LCFGSFD,   la_cfg_ex, {&lrd, &lrfa} },
{"lcfgvadrv", MATCH_LCFGVADRV, MASK_LCFGVADRV, la_cfg_ex, {&lrd, &lrxa} },
{"lcfgvadrs", MATCH_LCFGVADRS, MASK_LCFGVADRS, la_cfg_ex, {&lrd, &lrxa, &lrxb, &lrxc} },
{"lcfgvadrt", MATCH_LCFGVADRT, MASK_LCFGVADRT, la_cfg_ex, {&lrd, &lrxa, &lrxb, &lrxc} },
{"lcfgvlays", MATCH_LCFGVLAYS, MASK_LCFGVLAYS, la_cfg_ex, {&lrd, &lrxa, &lrxb, &lrxc} },
{"lcfgvlayd", MATCH_LCFGVLAYD, MASK_LCFGVLAYD, la_cfg_ex, {&lrd, &lrxa, &lrxb, &lrxc} },

//XFER OPS
{"lxferdata",     MATCH_LXFERDATA,     MASK_LXFERDATA,     la_xfer_ex, {&lrd, &lra, &lrxa} },
{"lxfercsrget",   MATCH_LXFERCSRGET,   MASK_LXFERCSRGET,   la_xfer_ex, {&lrxa} },
{"lxfercsrclear", MATCH_LXFERCSRCLEAR, MASK_LXFERCSRCLEAR, la_xfer_ex, {} },

};

} //namespace la_core

#endif // __LA_INSN__
