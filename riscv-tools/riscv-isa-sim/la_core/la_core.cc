//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "la_core.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "la_insn.h"
#include "la_mmu.h"
#include "la_scratch.h"
#include "la_types.h"

namespace la_core {

//=============================================================================
// constructor/destructor
//=============================================================================

LACore::LACore() :
  extension_t()
{
  char * scratch_size_str;
  if((scratch_size_str = std::getenv("SCRATCH_SIZE")) != nullptr) {
    uint64_t size = std::stoi(std::string(scratch_size_str));
    if(size == 0) {
      std::cout << "invalid SCRATCH_SIZE env var\n";
      exit(0);
    }
    std::cout << "scratch_size is " << (1 << size) << "\n";
    scratch = new LAScratch(1 << size);
    mmu = new LAMMU(p, scratch);
  } else {
    std::cout << "invalid SCRATCH_SIZE env var\n";
    exit(0);
  }
}

LACore::~LACore()
{
}

//=============================================================================
// extension_t overrides
//=============================================================================

std::vector<insn_desc_t>
LACore::get_instructions()
{ 
  std::vector<insn_desc_t> insns;
  for (auto& desc : la_insn_descs) {
    insn_desc_t new_desc = {
      desc.match, desc.mask, &::illegal_instruction, desc.func
    };
    insns.push_back(new_desc);
  }
  return insns;
}

std::vector<disasm_insn_t*> 
LACore::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  for (auto& desc : la_insn_descs) {
    insns.push_back(new disasm_insn_t(desc.name, desc.match, desc.mask,
      desc.args));
  }
  return insns;
}

const char *
LACore::name() 
{
  return "la_core";
}

//=============================================================================
// Data Ops
//=============================================================================

//TODO: sparse incomplete (NOT YET IMPLEMENTED)
void
LACore::la_data_ex(LAInsn& insn)
{
  bool is_reduce = (insn.lop() == 0);
  LAReg* srcA    = &(laRegs[insn.lra()]);
  LAReg* srcB    = &(laRegs[insn.lrb()]);
  LAReg* srcC    = &(laRegs[insn.lrc()]);
  LAReg* dst     = &(laRegs[insn.lrd()]);
  LACount count = READ_REG(insn.lrxa());

  //no-ops are fine
  if(count == 0){
    return;
  }

  //if D is scalar, must be single-out, single-stream
  assert(dst->vector() || (insn.lop() == 0 && !insn.mst()));

  bool A_dp = srcA->dp();
  bool B_dp = srcB->dp();
  bool C_dp = srcC->dp();
  bool D_dp = dst->dp();

  uint64_t DST_SIZE = D_dp ? sizeof(double) : sizeof(float);

  uint64_t srcA_bytes = count*(A_dp ? sizeof(double) : sizeof(float));
  uint64_t srcB_bytes = count*(B_dp ? sizeof(double) : sizeof(float));
  uint64_t srcC_bytes = count*(C_dp ? sizeof(double) : sizeof(float));
  uint64_t dst_bytes  = count*(DST_SIZE);

  LACount Acount = (srcA->vector() ? 
                      (srcA->sparse() ? srcA->jdxCnt() : srcA->vecCount()) :
                      0);
  LACount Bcount = (srcB->vector() ? 
                      (srcB->sparse() ? srcB->jdxCnt() : srcB->vecCount()) :
                      0);
  LACount Ccount = (srcC->vector() ? 
                      (srcC->sparse() ? srcC->jdxCnt() : srcC->vecCount()) :
                      0);

  //multi-stream partition count
  LACount mst_count = 0;

  //how many items will get written to dst by the end of the operation
  LACount dst_count = count;

  //adjust D output bytes if single-out or single-out/single-stream
  if(is_reduce){
    dst_count = 1;
    dst_bytes = dst_count*DST_SIZE;
    if(insn.mst()){
      mst_count = Acount;

      assert(!Bcount || !mst_count || (Bcount == mst_count));
      mst_count = (mst_count ? mst_count : Bcount);

      assert(!Ccount || !mst_count || (Ccount == mst_count));
      mst_count = (mst_count ? mst_count : Ccount);

      assert(mst_count && (count % mst_count == 0));
      dst_count =  (count/mst_count);
      dst_bytes = dst_count*DST_SIZE;
    }
  }

  //allocate the buffers
  char * Adata = (char *)malloc(srcA_bytes);
  char * Bdata = (char *)malloc(srcB_bytes);
  char * Cdata = (char *)malloc(srcC_bytes);
  char * Ddata = (char *)malloc(dst_bytes);

  //read the sources from memory
  mmu->read(Adata, count, srcA);
  mmu->read(Bdata, count, srcB);
  mmu->read(Cdata, count, srcC);

  //used for reduce instructions
  double accum_dp = 0.0;
  float  accum_sp = 0.0;

  for(LACount i=0; i<count; ++i){
    if(D_dp){
      double Aitem = (double)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
      double Bitem = (double)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
      double Citem = (double)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

      //(A+-B)*/C or (A*/B)+-C
      //t2 == 'terms' on top, not top-2 anymore!
      double top   = insn.t2() ? Aitem+(insn.su() ? -Bitem  : Bitem)
                               : Aitem*(insn.dv() ? 1/Bitem : Bitem);
      double Ditem = insn.t2() ? top*  (insn.dv() ? 1/Citem : Citem)
                               : top+  (insn.su() ? -Citem  : Citem);
                      
      if(is_reduce){
        //scalar output
        switch(insn.rdct()){
          case LAReduce::Min: accum_dp  = std::min(accum_dp, Ditem); break;
          case LAReduce::Max: accum_dp  = std::max(accum_dp, Ditem); break;
          case LAReduce::Sum: accum_dp += Ditem;                     break;
          default:            assert(false);                         break;
        }
        if(insn.mst() && ((i+1) % mst_count == 0)){
          ((double *)Ddata)[i/ mst_count] = accum_dp;
          accum_dp = 0;
        }
      } else {
        //vector output
        ((double *)Ddata)[i] = Ditem;
      }
    } else {
      float Aitem = (float)(A_dp ? ((double*)Adata)[i] : ((float*)Adata)[i]);
      float Bitem = (float)(B_dp ? ((double*)Bdata)[i] : ((float*)Bdata)[i]);
      float Citem = (float)(C_dp ? ((double*)Cdata)[i] : ((float*)Cdata)[i]);

      //(A+-B)*/C or (A*/B)+-C
      //t2 == 'terms' on top, not top-2 anymore!
      float top   = insn.t2() ? Aitem+(insn.su() ? -Bitem  : Bitem)
                              : Aitem*(insn.dv() ? 1/Bitem : Bitem);
      float Ditem = insn.t2() ? top*  (insn.dv() ? 1/Citem : Citem)
                              : top+  (insn.su() ? -Citem  : Citem);
                      
      if(is_reduce){
        //scalar output
        switch(insn.rdct()){
          case LAReduce::Min: accum_sp  = std::min(accum_sp, Ditem); break;
          case LAReduce::Max: accum_sp  = std::max(accum_sp, Ditem); break;
          case LAReduce::Sum: accum_sp += Ditem;                     break;
          default:            assert(false);                         break;
        }
        if(insn.mst() && ((i+1) % mst_count == 0)){
          ((float *)Ddata)[i/ mst_count] = accum_sp;
          accum_sp = 0;
        }
      } else {
        //vector output
        ((float *)Ddata)[i] = Ditem;
      }
    }
  }
  if(is_reduce && !insn.mst()){
    if(D_dp){
      ((double *)Ddata)[0] = accum_dp;
    } else {
      ((float *)Ddata)[0] = accum_sp;
    }
  }

  mmu->write(Ddata, dst_count, dst);

  free(Adata);
  free(Bdata);
  free(Cdata);
  free(Ddata);
}

//============================================================================
// Cfg Ops
//============================================================================

void
LACore::la_cfg_ex(LAInsn& insn)
{
  LAReg* dst = &(laRegs[insn.lrd()]);

  //set the flags. not all flags are updated on every cfg!
  bool has_dp         = (!insn.vec() || (insn.vec() && insn.alt()));
  bool has_vector     = true;
  bool has_sparse     = (insn.vec() == 1 && insn.alt() == 0);
  bool has_transpose  = (has_sparse && insn.spv());

  dst->dp(       has_dp         ? insn.dp()  : dst->dp());
  dst->vector(   has_vector     ? insn.vec() : dst->vector());
  dst->sparse(   has_sparse     ? insn.spv() : dst->sparse());
  dst->transpose(has_transpose  ? insn.tns() : dst->transpose());

  //set the scalar values if applicable
  if(!insn.vec()){
    dst->loc((LALoc)insn.lrdloc());
    if(!insn.alt()){
      dst->scalarAddr(READ_REG(insn.lrxa()));
    } else if(insn.dp()){
      uint64_t tmp_dp = READ_FREG(insn.lrfa());
      double val_dp;
      memcpy((char *)&val_dp, (char *)&tmp_dp, sizeof(double));
      dst->scalarDouble(val_dp);
    } else {
      uint64_t tmp_fp = READ_FREG(insn.lrfa());
      float val_fp;
      memcpy((char *)&val_fp, (char *)&tmp_fp, sizeof(float));
      dst->scalarFloat(val_fp);
    }
  }

  //set the vector addr field if applicable
  if(insn.vec() && !insn.alt() && !insn.spv()){
    dst->vecAddr((LAAddr)READ_REG(insn.lrxa()));
  }

  //set the sparse addr fields if applicable
  if(insn.vec() && !insn.alt() && insn.spv()){
    dst->dataPtr((LAAddr)READ_REG(insn.lrxa()));
    dst->idxPtr((LAAddr)READ_REG(insn.lrxb()));
    dst->jdxPtr((LAAddr)READ_REG(insn.lrxc()));
  }

  //set the layout if applicable (sparse and vector redundant here, so only
  //set the vecotr one!!
  if(insn.vec() && insn.alt()){
    dst->loc(insn.lrdloc());
    dst->vecStride((LAVecStride)READ_REG(insn.lrxa()));
    dst->vecCount((LAVecCount)READ_REG(insn.lrxb()));
    dst->vecSkip((LAVecSkip)READ_REG(insn.lrxc()));
  }
}

//============================================================================
// Xfer Ops
//============================================================================

//TODO: literally zero error handling.
void
LACore::la_xfer_ex(LAInsn& insn)
{
  LAReg* dst = &(laRegs[insn.lrd()]);
  LAReg* src = &(laRegs[insn.lra()]);

  LACount count;
  uint64_t src_bytes;
  uint64_t dst_bytes;
  char *   src_data;
  char *   dst_data;
  double * src_tmp_dp;
  double * dst_tmp_dp;
  float *  src_tmp_fp;
  float *  dst_tmp_fp;

  switch(insn.lfunc3()){
    case 1:
      count = READ_REG(insn.lrxa());
      src_bytes = count*(src->dp() ? sizeof(double) : sizeof(float));
      dst_bytes = count*(dst->dp() ? sizeof(double) : sizeof(float));
      src_data = (char *)malloc(src_bytes);
      dst_data = (char *)malloc(dst_bytes);

      mmu->read(src_data, count, src);
      if(src->dp() && !dst->dp()){
        src_tmp_dp = (double *)src_data;
        dst_tmp_fp = (float *)dst_data;
        for(LACount i=0; i<count; ++i){
          dst_tmp_fp[i] = (float)src_tmp_dp[i];
        }
      } else if(!src->dp() && dst->dp()) {
        src_tmp_fp = (float *)src_data;
        dst_tmp_dp = (double *)dst_data;
        for(LACount i=0; i<count; ++i){
          dst_tmp_dp[i] = (double)src_tmp_fp[i];
        }
      } else {
        memcpy(dst_data, src_data, src_bytes);
      }
      mmu->write(dst_data, count, dst);

      free(src_data);
      free(dst_data);
      break;

    case 2:   WRITE_REG(insn.lrxa(), laCsrReg.readAll()); break;
    case 4:   laCsrReg.clearAll();                        break;
    default:  assert(false);                              break;
  }
}

} //namespace la_core
