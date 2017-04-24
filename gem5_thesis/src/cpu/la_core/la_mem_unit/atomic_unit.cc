//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "cpu/la_core/la_mem_unit/atomic_unit.hh"

#include <cassert>
#include <cstdint>
#include <cstring>

#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LAMemUnitAtomic::LAMemUnitAtomic()
{
}

LAMemUnitAtomic::~LAMemUnitAtomic()
{
}

//=============================================================================
// Public Interface Read/Write methods
//=============================================================================

LASpvCount
LAMemUnitAtomic::spv_read_idx_jdx_array(LAReg *reg, LAAddr array_ptr, 
    LACount offset, LACoreExecContext * xc)
{
  LAAddr SIZE = sizeof(LASpvCount);
  LAAddr addr = array_ptr + offset*SIZE;
  uint8_t data[SIZE];

  switch(reg->loc()){
    case LALoc::Mem:
      assert(xc->readLAMemAtomic(addr, data, SIZE) == NoFault);
      break;
    case LALoc::Sch:
      assert(xc->readLAScratchAtomic(addr, data, SIZE) == NoFault);
      break;
    default: 
      assert(false);
      break;
  }
  return *(LASpvCount*)data;
}

float
LAMemUnitAtomic::spv_read_sp_array(LAReg *reg, LAAddr array_ptr, 
    LACount offset, LACoreExecContext * xc)
{
  LAAddr SIZE = sizeof(float);
  LAAddr addr = array_ptr + offset*SIZE;
  uint8_t data[SIZE];

  switch(reg->loc()){
    case LALoc::Mem:
      assert(xc->readLAMemAtomic(addr, data, SIZE) == NoFault);
      break;
    case LALoc::Sch:
      assert(xc->readLAScratchAtomic(addr, data, SIZE) == NoFault);
      break;
    default: 
      assert(false);
      break;
  }
  return *(float *)data;
}

double
LAMemUnitAtomic::spv_read_dp_array(LAReg *reg, LAAddr array_ptr, 
    LACount offset, LACoreExecContext * xc)
{
  LAAddr SIZE = sizeof(double);
  LAAddr addr = array_ptr + offset*SIZE;
  uint8_t data[SIZE];

  switch(reg->loc()){
    case LALoc::Mem:
      assert(xc->readLAMemAtomic(addr, data, SIZE) == NoFault);
      break;
    case LALoc::Sch:
      assert(xc->readLAScratchAtomic(addr, data, SIZE) == NoFault);
      break;
    default: 
      assert(false);
      break;
  }
  return *(double *)data;
}

void 
LAMemUnitAtomic::readAtomic(uint8_t *data, LACount count, LAReg * reg,
  LACoreExecContext * xc)
{
    assert(count > 0);
    uint8_t SIZE = reg->dp() ? sizeof(double) : sizeof(float);

    if(!reg->vector()){
        LAAddr addr = reg->scalarAddr();
        switch(reg->loc()){
            case LALoc::Reg: 
                if(reg->dp()){
                    (*(double *)data) = reg->scalarDouble();
                } else {
                    (*(float *)data) = reg->scalarFloat();
                }
                break;
            case LALoc::Mem: 
                assert(xc->readLAMemAtomic(addr, data, SIZE) == NoFault);
                break;
            case LALoc::Sch:
                assert(xc->readLAScratchAtomic(addr, data, SIZE) == NoFault);
                break;
            default:
                assert(false);
                break;
        }
        for(LACount i=1; i<count; ++i){
            memcpy(data+(i*SIZE), data, SIZE);
        }
    } else if (!reg->sparse()) {
        LAAddr      vaddr       = reg->vecAddr();
        LAVecStride vstride     = reg->vecStride();
        LAVecCount  vcount      = reg->vecCount(); 
        LAVecSkip   vskip       = reg->vecSkip();

        for(LACount i=0; i<count; i++){
            LAAddr addr = vaddr + SIZE*(vstride*i + vskip*(i/vcount));
            switch(reg->loc()){
                case LALoc::Mem:
                    assert(xc->readLAMemAtomic(addr, data, SIZE) == NoFault);
                    break;
                case LALoc::Sch:
                    assert(xc->readLAScratchAtomic(addr, data, SIZE)==NoFault);
                    break;
                default: 
                    assert(false);
                    break;
            }
            data += SIZE;
        }
    } else {
      LAAddr      data_ptr  =  reg->dataPtr();
      LAAddr      idx_ptr   =  reg->idxPtr();
      LAAddr      jdx_ptr   =  reg->jdxPtr();
      LASpvCount  jdx_cnt   =  reg->jdxCnt();
      LASpvCount  data_skip =  reg->dataSkip();
      double *    data_tmp_dp = (double *) data;
      float *     data_tmp_sp = (float *) data;
  
      for(LACount i=0; i<count; i++){
        LASpvCount virt_idx = (i+data_skip) / jdx_cnt;
        LASpvCount virt_jdx = (i+data_skip) % jdx_cnt;
  
        LASpvCount idx_val_cur = (virt_idx == 0 ? 0 :
          spv_read_idx_jdx_array(reg, idx_ptr, virt_idx-1, xc));
        LASpvCount idx_val_nxt =
          spv_read_idx_jdx_array(reg, idx_ptr, virt_idx, xc);
        LASpvCount idx_elems = idx_val_nxt - idx_val_cur;
        
        bool found = false;
        if(idx_elems > 0) {
          for(LASpvCount jdx_offset=0; jdx_offset<idx_elems; ++jdx_offset) {
            LASpvCount jdx_val = 
              spv_read_idx_jdx_array(reg, jdx_ptr, idx_val_cur+jdx_offset, xc);
            if(jdx_val == virt_jdx) {
              if(reg->dp()){
                data_tmp_dp[i] =
                  spv_read_dp_array(reg, data_ptr, idx_val_cur+jdx_offset, xc);
              } else {
                data_tmp_sp[i] = 
                  spv_read_sp_array(reg, data_ptr, idx_val_cur+jdx_offset, xc);
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
LAMemUnitAtomic::writeAtomic(uint8_t *data, LACount count, LAReg * reg,
  LACoreExecContext * xc)
{
    assert(count > 0);
    uint8_t SIZE = reg->dp() ? sizeof(double) : sizeof(float);

    if(!reg->vector()){
        assert(count == 1);
        LAAddr addr = reg->scalarAddr();
        switch(reg->loc()){
            case LALoc::Reg: 
                if (reg->dp()) {
                    reg->scalarDouble(*(double *)data);
                } else {
                    reg->scalarFloat(*(float *)data);
                }
                break;
            case LALoc::Mem:
                assert(xc->writeLAMemAtomic(addr, data, SIZE) == NoFault);
                break;
            case LALoc::Sch:
                assert(xc->writeLAScratchAtomic(addr, data, SIZE) == NoFault);
                break;
            default:
              assert(false);
              break;
        }
    } else if (!reg->sparse()) {
        LAAddr      vaddr   = reg->vecAddr();
        LAVecStride vstride = reg->vecStride();
        LAVecCount  vcount  = reg->vecCount(); 
        LAVecSkip   vskip   = reg->vecSkip();

        for(LACount i=0; i<count; i++){
            LAAddr addr = vaddr + SIZE*(vstride*i + vskip*(i/vcount));
            switch(reg->loc()){
                case LALoc::Mem: 
                    assert(xc->writeLAMemAtomic(addr, data, SIZE)==NoFault);
                    break;
                case LALoc::Sch:
                    assert(xc->writeLAScratchAtomic(addr, data, SIZE)==NoFault);
                    break;
                default:
                    assert(false);
                    break;
            }
            data += SIZE;
        }
    } else {
        //TODO: sparse (NOT YET IMPLEMENTED)
        assert(false);
    }
}
