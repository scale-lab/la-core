//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "la_mmu.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#include "decode.h"
#include "la_core.h"
#include "la_scratch.h"
#include "mmu.h"

namespace la_core {

//=============================================================================
// constructor/destructor
//=============================================================================

LAMMU::LAMMU(processor_t*& p, LAScratch * scratch) :
  p(p), scratch(scratch)
{
}

LAMMU::~LAMMU()
{
}

//=============================================================================
// Public Interface Read/Write methods
//=============================================================================

LASpvCount
LAMMU::spv_read_idx_jdx_array(LAReg *reg, LAAddr array_ptr, LACount offset)
{
  mmu_t *cpuMMU = p->get_mmu();

  uint32_t tmp_val;
  switch(reg->loc()){
    case LALoc::Mem:
      tmp_val = cpuMMU->load_uint32(array_ptr + offset*sizeof(LASpvCount));
      break;
    case LALoc::Sch:
      tmp_val = scratch->readUint32(array_ptr + offset*sizeof(LASpvCount));
      break;
    default: 
      assert(false);
      break;
  }
  return (LASpvCount)tmp_val;
}

float
LAMMU::spv_read_sp_array(LAReg *reg, LAAddr array_ptr, LACount offset)
{
  mmu_t *cpuMMU = p->get_mmu();

  float val_fp;
  uint32_t tmp_fp;

  switch(reg->loc()){
    case LALoc::Mem:
      tmp_fp = cpuMMU->load_uint32(array_ptr + offset*sizeof(float));
      memcpy((char *)&val_fp, (char *)&tmp_fp, sizeof(float));
      break;
    case LALoc::Sch:
      tmp_fp = scratch->readUint32(array_ptr + offset*sizeof(float));
      memcpy((char *)&val_fp, (char *)&tmp_fp, sizeof(float));
      break;
    default: 
      assert(false);
      break;
  }
  return val_fp;
}

double
LAMMU::spv_read_dp_array(LAReg *reg, LAAddr array_ptr, LACount offset)
{
  mmu_t *cpuMMU = p->get_mmu();

  double val_dp;
  uint64_t tmp_dp;

  switch(reg->loc()){
    case LALoc::Mem:
      tmp_dp = cpuMMU->load_uint64(array_ptr + offset*sizeof(double));
      memcpy((char *)&val_dp, (char *)&tmp_dp, sizeof(double));
      break;
    case LALoc::Sch:
      tmp_dp = scratch->readUint64(array_ptr + offset*sizeof(double));
      memcpy((char *)&val_dp, (char *)&tmp_dp, sizeof(double));
      break;
    default: 
      assert(false);
      break;
  }
  return val_dp;
}



void 
LAMMU::read(char *data, LACount count, LAReg * reg)
{
  mmu_t *cpuMMU = p->get_mmu();

  double val_dp;
  uint64_t tmp_dp;
  float val_fp;
  uint32_t tmp_fp;

  if(!reg->vector()){
    LAAddr addr = reg->scalarAddr();
    if(reg->dp()){
      switch(reg->loc()){
        case LALoc::Reg: 
          val_dp = reg->scalarDouble();                        
          break;
        case LALoc::Mem: 
          tmp_dp = cpuMMU->load_uint64(addr);
          memcpy((char *)&val_dp, (char *)&tmp_dp, sizeof(double));
          break;
        case LALoc::Sch:
          tmp_dp = scratch->readUint64(addr);
          memcpy((char *)&val_dp, (char *)&tmp_dp, sizeof(double));
          break;
        default:
          assert(false);
          break;
      }
      double *data_tmp_dp = (double *)data;
      for(LACount i=0; i<count; ++i){
        data_tmp_dp[i] = val_dp;
      } 
    } else {
      switch(reg->loc()){
        case LALoc::Reg:
          val_fp = reg->scalarFloat();
          break;
        case LALoc::Mem:
          tmp_fp = cpuMMU->load_uint32(addr);
          memcpy((char *)&val_fp, (char *)&tmp_fp, sizeof(float));
          break;
        case LALoc::Sch:
          tmp_fp = scratch->readUint32(addr);
          memcpy((char *)&val_fp, (char *)&tmp_fp, sizeof(float));
          break;
        default:  
          assert(false);
          break;
      }
      float *data_tmp_sp = (float *)data;
      for(LACount i=0; i<count; ++i){
        data_tmp_sp[i] = val_fp;
      }
    }
  } else if(!reg->sparse()){
    LAAddr      vaddr       = reg->vecAddr();
    LAVecStride vstride     = reg->vecStride();
    LAVecCount  vcount      = reg->vecCount(); 
    LAVecSkip   vskip       = reg->vecSkip();
    double *    data_tmp_dp = (double *) data;
    float *     data_tmp_sp = (float *) data;
    char        SIZE        = reg->dp() ? sizeof(double) : sizeof(float);

    for(LACount i=0; i<count; i++){
      LAAddr addr = vaddr + SIZE*(vstride*i + vskip*(i/vcount));
      if(reg->dp()){
        switch(reg->loc()){
          case LALoc::Mem:
            tmp_dp = cpuMMU->load_uint64(addr);
            memcpy((char *)&(data_tmp_dp[i]), (char *)&tmp_dp, sizeof(double));
            break;
          case LALoc::Sch:
            tmp_dp = scratch->readUint64(addr);
            memcpy((char *)&(data_tmp_dp[i]), (char *)&tmp_dp, sizeof(double));
            break;
          default: 
            assert(false);
            break;
        }
      } else {
        switch(reg->loc()){
          case LALoc::Mem:
            tmp_fp = cpuMMU->load_uint32(addr);
            memcpy((char *)&(data_tmp_sp[i]), (char *)&tmp_fp, sizeof(float));
            break;
          case LALoc::Sch:
            tmp_fp = scratch->readUint32(addr);
            memcpy((char *)&(data_tmp_sp[i]), (char *)&tmp_fp, sizeof(float));
            break;
          default:
            assert(false);
            break;
        }
      }
    }
  } else {
    LAAddr      data_ptr  =  reg->dataPtr();
    LAAddr      idx_ptr   =  reg->idxPtr();
    LAAddr      jdx_ptr   =  reg->jdxPtr();
    LASpvCount  idx_cnt   =  reg->idxCnt();
    LASpvCount  jdx_cnt   =  reg->jdxCnt();
    LASpvCount  data_skip =  reg->dataSkip();
    double *    data_tmp_dp = (double *) data;
    float *     data_tmp_sp = (float *) data;
    char        SIZE        = reg->dp() ? sizeof(double) : sizeof(float);

    for(LACount i=0; i<count; i++){
      LASpvCount virt_idx = (i+data_skip) / jdx_cnt;
      LASpvCount virt_jdx = (i+data_skip) % jdx_cnt;

      LASpvCount idx_val_cur = (virt_idx == 0 ? 0 :
        spv_read_idx_jdx_array(reg, idx_ptr, virt_idx-1));
      LASpvCount idx_val_nxt = spv_read_idx_jdx_array(reg, idx_ptr, virt_idx);
      LASpvCount idx_elems = idx_val_nxt - idx_val_cur;
      
      bool found = false;
      if(idx_elems > 0) {
        for(LASpvCount jdx_offset=0; jdx_offset<idx_elems; ++jdx_offset) {
          LASpvCount jdx_val = 
            spv_read_idx_jdx_array(reg, jdx_ptr, idx_val_cur+jdx_offset);
          if(jdx_val == virt_jdx) {
            if(reg->dp()){
              data_tmp_dp[i] =
                spv_read_dp_array(reg, data_ptr, idx_val_cur+jdx_offset);
            } else {
              data_tmp_sp[i] = 
                spv_read_sp_array(reg, data_ptr, idx_val_cur+jdx_offset);
            }
            found = true;
            break;
          }
        }
      }
      if(!found) {
        if(reg->dp()){
          data_tmp_dp[i] = 0.0;
        } else {
          data_tmp_sp[i] = 0.0;
        }
      }  
    }
  }
}

//TODO: sparse vectors (NOT YET IMPLEMENTED)
void 
LAMMU::write(char *data, LACount count, LAReg * reg)
{
  mmu_t *cpuMMU = p->get_mmu();

  double val_dp;
  uint64_t tmp_dp;
  float val_fp;
  uint32_t tmp_fp;

  if(!reg->vector()){
    assert(count == 1);
    LAAddr addr = reg->scalarAddr();
    if(reg->dp()){
      switch(reg->loc()){
        case LALoc::Reg: 
          memcpy((char *)&val_dp, data, sizeof(double));
          reg->scalarDouble(val_dp);
          break;
        case LALoc::Mem:
          memcpy((char *)&tmp_dp, data, sizeof(double));
          cpuMMU->store_uint64(addr, tmp_dp);
          break;
        case LALoc::Sch:
          memcpy((char *)&tmp_dp, data, sizeof(double));
          scratch->writeUint64(addr, tmp_dp);
          break;
        default:
          assert(false);
          break;
      }
    } else {
      switch(reg->loc()){
        case LALoc::Reg:
          memcpy((char *)&val_fp, data, sizeof(float));
          reg->scalarFloat(val_fp);
          break;
        case LALoc::Mem:
          memcpy((char *)&tmp_fp, data, sizeof(float));
          cpuMMU->store_uint32(addr, tmp_fp);
          break;
        case LALoc::Sch:
          memcpy((char *)&tmp_fp, data, sizeof(float));
          scratch->writeUint32(addr, tmp_fp);
          break;
        default:
          assert(false);
          break;
      }
    }
  } else if(!reg->sparse()){
    LAAddr      vaddr       = reg->vecAddr();
    LAVecStride vstride     = reg->vecStride();
    LAVecCount  vcount      = reg->vecCount(); 
    LAVecSkip   vskip       = reg->vecSkip();
    double *    data_tmp_dp = (double *) data;
    float *     data_tmp_sp = (float *) data;
    char        SIZE        = reg->dp() ? sizeof(double) : sizeof(float);

    for(LACount i=0; i<count; i++){
      LAAddr addr = vaddr + SIZE*(vstride*i + vskip*(i/vcount));
      if(reg->dp()){
        double d = data_tmp_dp[i];
        switch(reg->loc()){
          case LALoc::Mem: 
            memcpy((char *)&tmp_dp, (char *)&d, sizeof(double));
            cpuMMU->store_uint64(addr, tmp_dp);
            break;
          case LALoc::Sch:
            memcpy((char *)&tmp_dp, (char *)&d, sizeof(double));
            scratch->writeUint64(addr, tmp_dp);
            break;
          default:
            assert(false);
            break;
        }
      } else {
        float f = data_tmp_sp[i];
        switch(reg->loc()){
          case LALoc::Mem:
            memcpy((char *)&tmp_fp, (char *)&f, sizeof(float));
            cpuMMU->store_uint32(addr, tmp_fp);
            break;
          case LALoc::Sch:
            memcpy((char *)&tmp_fp, (char *)&f, sizeof(float));
            scratch->writeUint32(addr, tmp_fp);
            break;
          default:
            assert(false);
            break;
        }
      }
    }
  } else {
    //TODO: sparse (NOT YET IMPLEMENTED)
    assert(false);
  }
}

} // namespace la_core
