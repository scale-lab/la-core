//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __CPU_LA_CORE_LA_REG_H__
#define __CPU_LA_CORE_LA_REG_H__

#include <bitset>
#include <cstdint>

#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_types.hh"

//=============================================================================
// LAReg Registers
//=============================================================================

class LAReg
{
public:
  LAReg() : data0({.a=0}), data1(0), data2(0), 
    layout0({.vs=0}), layout1({.vc=0}), layout2({.vs=0}),
    _dp(false), _vector(false), _sparse(false), _transpose(false), 
    _loc(LALoc::Reg) {}
  ~LAReg() {}

  //scalar getters/setters
  LAAddr scalarAddr(){   return data0.a; }
  float  scalarFloat(){  return data0.f; }
  double scalarDouble(){ return data0.d; }

  void scalarAddr(LAAddr val){   data0.a = val; }
  void scalarFloat(float val){   data0.f = val; }
  void scalarDouble(double val){ data0.d = val; }

  //vector getters/setters
  LAAddr      vecAddr(){   return data0.a; }
  LAVecStride vecStride(){ return layout0.vs; }
  LAVecCount  vecCount(){  return layout1.vc; }
  LAVecSkip   vecSkip(){   return layout2.vs; }

  void vecAddr(LAAddr val){        data0.a = val; }
  void vecStride(LAVecStride val){ layout0.vs = val; }
  void vecCount(LAVecCount val){   layout1.vc = val; }
  void vecSkip(LAVecSkip val){     layout2.vs = val; }

  //sparse getters/setters
  LAAddr     dataPtr(){  return data0.a; }
  LAAddr     idxPtr(){   return data1; }
  LAAddr     jdxPtr(){   return data2; }
  LASpvCount idxCnt(){   return layout0.ic; }
  LASpvCount jdxCnt(){   return layout1.jc; }
  LASpvCount dataSkip(){ return layout2.ds; }

  void dataPtr(LAAddr val){      data0.a = val; }
  void idxPtr(LAAddr val){       data1 = val; }
  void jdxPtr(LAAddr val){       data2 = val; }
  void idxCnt(LASpvCount val){   layout0.ic = val; }
  void jdxCnt(LASpvCount val){   layout1.jc = val; }
  void dataSkip(LASpvCount val){ layout2.ds = val; }

  //flag getters/setters
  bool dp(){        return _dp; }
  bool vector() {   return _vector; }
  bool sparse() {   return _sparse; }
  bool transpose(){ return _transpose; }

  void dp(bool val){        _dp         = val; }
  void vector(bool val) {   _vector     = val; }
  void sparse(bool val) {   _sparse     = val; }
  void transpose(bool val){ _transpose  = val; }

  //loc getters/setters
  LALoc loc(){ return _loc; }
  void loc(LALoc val){ _loc = val; }

private:
  union data0 { float f; double d; LAAddr a; } data0;
  uint64_t data1;
  uint64_t data2;
  union layout0 { LAVecStride vs; LASpvCount ic; } layout0;
  union layout1 { LAVecCount  vc; LASpvCount jc; } layout1;
  union layout2 { LAVecSkip   vs; LASpvCount ds; } layout2;
  bool _dp;
  bool _vector;
  bool _sparse;
  bool _transpose;
  LALoc _loc;
};

//=============================================================================
// LACore Control-Status Register
//=============================================================================

//control-status register
class LACsrReg
{
 public:
  LACsrReg() {}
  ~LACsrReg() {}

  bool getFlag(LACsrFlag flag){ return flags[flag]; }
  void setFlag(LACsrFlag flag, bool val){ flags[flag] = val; }

  uint64_t readAll(){ return flags.to_ullong(); }
  void clearAll() { flags.reset(); }

private:
  std::bitset<NumLACsrFlags> flags;
};


#endif //__CPU_LA_CORE_LA_REG_H__
