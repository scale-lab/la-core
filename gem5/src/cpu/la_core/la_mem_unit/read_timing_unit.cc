//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "cpu/la_core/la_mem_unit/read_timing_unit.hh"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>

#include "base/types.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"
#include "debug/LAMemUnitReadTiming.hh"
#include "params/LAMemUnitReadTiming.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LAMemUnitReadTiming::LAMemUnitReadTiming(LAMemUnitReadTimingParams *p) :
    TickedObject(p), channel(p->channel), cacheLineSize(p->cacheLineSize),
    scratchLineSize(p->scratchLineSize), done(false)
{
}

LAMemUnitReadTiming::~LAMemUnitReadTiming()
{
}

//=============================================================================
// Overrides
//=============================================================================

void
LAMemUnitReadTiming::evaluate()
{
    //Do NOT assert if done here! it coud've finished in < 1 cycle!
    assert(running);
    if (!done) { readFunction(); }
    if (done) {
        done = false;
        stop();
    }
}

void
LAMemUnitReadTiming::regStats()
{
  TickedObject::regStats();
  ClockedObject::regStats();
}


//=============================================================================
// main functions
//=============================================================================


// NOTE: any data this is passed to 'on_item_load' MUST be delete[]ed by the
// consumer! the ReadUnit will NOT delete this data it allocates!
void
LAMemUnitReadTiming::initialize(LACount count, LAReg *reg,
  LACoreExecContext *xc,
  std::function<void(uint8_t*,uint8_t,bool)> on_item_load)
{
    assert(!running && !done);
    assert(count > 0);

    uint8_t SIZE = reg->dp() ? sizeof(double) : sizeof(float);

    if(!reg->vector()){
        LAAddr addr = reg->scalarAddr();
        auto fin = [on_item_load,SIZE,count,this](uint8_t*data, uint8_t size){
            assert(size == SIZE);
            for (LACount i=0; i<count; ++i) {
                bool _done = ((i+1) == count);
                uint8_t *ndata = new uint8_t[SIZE];
                memcpy(ndata, data, SIZE);
                on_item_load(ndata, SIZE, _done);
                DPRINTF(LAMemUnitReadTiming,
                    "calling on_item_load with size %d. 'done'=%d\n",
                    size, _done);
            }
        };
        switch(reg->loc()){
            case LALoc::Reg: 
                readFunction = [SIZE,reg,fin,this](){
                    uint8_t*data = new uint8_t[SIZE];
                    if(reg->dp()){
                        double tmp_dp = reg->scalarDouble();
                        memcpy(data, (uint8_t*)&tmp_dp, SIZE);
                    } else {
                        float tmp_fp = reg->scalarFloat();
                        memcpy(data, (uint8_t*)&tmp_fp, SIZE);
                    }
                    fin(data, SIZE);
                    delete[] data;
                    this->done = true;
                    return this->done;
                };
                break;
            case LALoc::Mem: 
                readFunction = [xc,addr,fin,SIZE,this](){
                    this->done = xc->readLAMemTiming(addr, SIZE, channel, fin);
                    return this->done;
                };
                break;
            case LALoc::Sch:
                readFunction = [xc,addr,fin,SIZE,this](){
                    this->done =
                        xc->readLAScratchTiming(addr, SIZE, channel, fin);
                    return this->done;
                };
                break;
            default: 
                assert(false);
                break;
        }
    } else if(!reg->sparse()){
        LAAddr      vaddr       = reg->vecAddr();
        LAVecStride vstride     = reg->vecStride();
        LAVecCount  vcount      = reg->vecCount(); 
        LAVecSkip   vskip       = reg->vecSkip();

        //reset vecIndex
        vecIndex = 0;

        auto fin = [on_item_load,SIZE,count,this]
            (LACount i, std::vector<uint64_t> line_offsets) 
        {
            return [on_item_load,SIZE,count,i,line_offsets,this]
                (uint8_t*data, uint8_t size) 
            {
                for (uint64_t j=0; j< line_offsets.size(); ++j) {
                    bool _done = ( (i+j+1) == count );
                    uint8_t *ndata = new uint8_t[SIZE];
                    memcpy(ndata, data + line_offsets[j], SIZE);
                    DPRINTF(LAMemUnitReadTiming,
                        "calling on_item_load with size %d. 'done'=%d\n",
                        SIZE, _done);
                    on_item_load(ndata, SIZE, _done);
                }
            };
        };
        
        readFunction = [reg,vaddr,vstride,vcount,vskip,SIZE,
            count,on_item_load,xc,fin,this]()
        {
            std::vector<uint64_t> line_offsets;

            //scratch and cache could use different line sizes
            uint64_t line_size;
            switch (reg->loc()) {
                case LALoc::Mem: line_size = cacheLineSize; break;
                case LALoc::Sch: line_size = scratchLineSize; break;
                default: panic("invalid reg->loc()"); break;
            }

            //we can always read the first item
            LACount i = this->vecIndex;
            LAAddr addr = vaddr + SIZE*(i*vstride + vskip*(i/vcount));
            LAAddr line_addr = addr - (addr % line_size);
            line_offsets.push_back(addr % line_size);
            uint8_t items_in_line = 1;

            //try to read more items in the same cache-line
            for (uint8_t j=1; j<(line_size/SIZE) && (i+j)<count; ++j) {
                LAAddr next_addr = vaddr +
                    SIZE*((i+j)*vstride + vskip*((i+j)/vcount));
                LAAddr next_line_addr = next_addr - (next_addr % line_size);
                if (next_line_addr == line_addr) {
                    items_in_line += 1;
                    line_offsets.push_back(next_addr % line_size);
                } else {
                    break;
                }
            }

            //try to read the line
            DPRINTF(LAMemUnitReadTiming,
                "reading line_addr %#x with %d items for addr %#x\n",
                line_addr, items_in_line, addr);

            bool success;
            if (reg->loc() == LALoc::Mem) {
                success = xc->readLAMemTiming(line_addr, line_size,
                    this->channel, fin(i, line_offsets));
            } else {
                success = xc->readLAScratchTiming(line_addr, line_size,
                    this->channel, fin(i, line_offsets));
            }
            if (success) {
                this->vecIndex += items_in_line;
                this->done = (this->vecIndex == count);
                return true;
            }
            return false;
        };
    } else {
        LAAddr      data_ptr  =  reg->dataPtr();
        LAAddr      idx_ptr   =  reg->idxPtr();
        LAAddr      jdx_ptr   =  reg->jdxPtr();
        LASpvCount  jdx_cnt   =  reg->jdxCnt();
        LASpvCount  data_skip =  reg->dataSkip();

        //reset vecIndex
        spvIndex        = 0;    //this is the total element 'count'

        gotIdxValCur    = false;
        gotIdxValNxt    = false;
        idxValCur       = 0;
        idxValNxt       = 0;

        jdxOffset       = 0;
        gotJdxVal       = false;
        jdxVal          = 0;

        gotDataVal      = false;
        dataVal         = nullptr;

        //local cache-like buffers
        spvCachedIdxLineAddr    = 0;
        spvCachedIdxLine        = nullptr;
        spvSentIdxReq           = false;

        spvCachedJdxLineAddr    = 0;
        spvCachedJdxLine        = nullptr;
        spvSentJdxReq           = false;

        spvCachedDataLineAddr   = 0;
        spvCachedDataLine       = nullptr;
        spvSentDataReq          = false;

        //----------------------------------------------------------------------
        auto spv_idx_fetch_fin = [this](LAAddr virt_idx_line_addr) {
            return [this,virt_idx_line_addr](uint8_t*data, uint8_t size) {
                if(this->spvCachedIdxLine != nullptr){
                    free(this->spvCachedIdxLine);
                }
                this->spvCachedIdxLine = (LASpvCount*)malloc(size);
                memcpy((uint8_t *)this->spvCachedIdxLine, data, size);

                this->spvCachedIdxLineAddr = virt_idx_line_addr;
                this->spvSentIdxReq = false;
            };
        };

        auto spv_jdx_fetch_fin = [this](LAAddr virt_jdx_line_addr) {
            return [this,virt_jdx_line_addr](uint8_t*data, uint8_t size) {
                if(this->spvCachedJdxLine != nullptr){
                    free(this->spvCachedJdxLine);
                }
                this->spvCachedJdxLine = (LASpvCount*)malloc(size);
                memcpy((uint8_t *)this->spvCachedJdxLine, data, size);

                this->spvCachedJdxLineAddr = virt_jdx_line_addr;
                this->spvSentJdxReq = false;
            };
        };

        auto spv_data_fetch_fin = [this](LAAddr data_line_addr) {
            return [this,data_line_addr](uint8_t*data, uint8_t size) {
                if(this->spvCachedDataLine != nullptr){
                    free(this->spvCachedDataLine);
                }
                this->spvCachedDataLine = (uint8_t*)malloc(size);
                memcpy((uint8_t *)this->spvCachedDataLine, data, size);

                this->spvCachedDataLineAddr = data_line_addr;
                this->spvSentDataReq = false;
            };
        };

        auto data_fin = [on_item_load,this](uint8_t*data, uint8_t size) {
            uint8_t *ndata = new uint8_t[size];
            memcpy(ndata, data, size);
            DPRINTF(LAMemUnitReadTiming,
                "calling on_item_load with size %d. 'done'=%d\n",
                size, this->done);
            on_item_load(ndata, size, this->done);
        };

        //----------------------------------------------------------------------

        readFunction = [reg,data_ptr,idx_ptr,jdx_ptr,jdx_cnt,data_skip,SIZE,
            count,spv_idx_fetch_fin,spv_jdx_fetch_fin,spv_data_fetch_fin,
            data_fin,xc,this]()
        {
            //scratch and cache could use different line sizes
            uint64_t line_size;
            switch (reg->loc()) {
                case LALoc::Mem: line_size = cacheLineSize; break;
                case LALoc::Sch: line_size = scratchLineSize; break;
                default: panic("invalid reg->loc()"); break;
            }
            LASpvCount virt_idx = (this->spvIndex+data_skip) / jdx_cnt;
            LASpvCount virt_jdx = (this->spvIndex+data_skip) % jdx_cnt;

            //------------------------------------------------------------------
            // idxValCur/idxValNxt
            //------------------------------------------------------------------
            LAAddr IDXSIZE = sizeof(LASpvCount);
            LAAddr virt_idx_cur_addr = idx_ptr + (virt_idx-1)*IDXSIZE;
            LAAddr virt_idx_nxt_addr = idx_ptr + (virt_idx)*IDXSIZE;

            LAAddr virt_idx_cur_line_offset = (virt_idx_cur_addr%line_size);
            LAAddr virt_idx_nxt_line_offset = (virt_idx_nxt_addr%line_size);

            LAAddr virt_idx_cur_line_addr = virt_idx_cur_addr -
                                            virt_idx_cur_line_offset;
            LAAddr virt_idx_nxt_line_addr = virt_idx_nxt_addr -
                                            virt_idx_nxt_line_offset;

            if (!gotIdxValCur) {
                if (virt_idx == 0) {
                    idxValCur = 0;
                    gotIdxValCur = true;
                } else if (spvCachedIdxLineAddr == virt_idx_cur_line_addr) {
                    idxValCur =
                        spvCachedIdxLine[virt_idx_cur_line_offset/IDXSIZE];
                    gotIdxValCur = true;
                } else if (!spvSentIdxReq) {
                    if (reg->loc() == LALoc::Mem) {
                        if(xc->readLAMemTiming(virt_idx_cur_line_addr,
                            line_size, this->channel,
                            spv_idx_fetch_fin(virt_idx_cur_line_addr))) 
                        {
                            spvSentIdxReq = true;
                        }
                        return false;
                    } else {
                        if (xc->readLAScratchTiming(virt_idx_cur_line_addr,
                            line_size, this->channel,
                            spv_idx_fetch_fin(virt_idx_cur_line_addr))) 
                        {
                            spvSentIdxReq = true;
                        }
                        return false;
                    }
                } else {
                    //we already have a request for idx line in flight
                    return false;
                }
            }
            //------------------------------------------------------------------
            if (!gotIdxValNxt) {
                if (spvCachedIdxLineAddr == virt_idx_nxt_line_addr) {
                    idxValNxt =
                        spvCachedIdxLine[virt_idx_nxt_line_offset/IDXSIZE];
                    gotIdxValNxt = true;
                } else if (!spvSentIdxReq) {
                    if (reg->loc() == LALoc::Mem) {
                        if (xc->readLAMemTiming(virt_idx_nxt_line_addr,
                            line_size, this->channel,
                            spv_idx_fetch_fin(virt_idx_nxt_line_addr)))
                        {
                            spvSentIdxReq = true;
                        }
                        return false;
                    } else {
                        if (xc->readLAScratchTiming(virt_idx_nxt_line_addr,
                            line_size, this->channel,
                            spv_idx_fetch_fin(virt_idx_nxt_line_addr)))
                        {
                            spvSentIdxReq = true;
                        }
                        return false;
                    }
                } else {
                    //we already have a request for idx line in flight
                    return false;
                }
            }
            
            LASpvCount idx_elems = idxValNxt - idxValCur;

            //------------------------------------------------------------------
            // jdxValCur
            //------------------------------------------------------------------
            LAAddr virt_jdx_addr = jdx_ptr + (idxValCur+jdxOffset)*IDXSIZE;
            LAAddr virt_jdx_line_offset = (virt_jdx_addr%line_size);
            LAAddr virt_jdx_line_addr = virt_jdx_addr - virt_jdx_line_offset;

            if (jdxOffset < idx_elems) {
                if (!gotJdxVal) {
                    if (spvCachedJdxLineAddr == virt_jdx_line_addr) {
                        jdxVal =
                            spvCachedJdxLine[virt_jdx_line_offset/IDXSIZE];
                        gotJdxVal = true;
                    } else if (!spvSentJdxReq) {
                        if (reg->loc() == LALoc::Mem) {
                            if (xc->readLAMemTiming(virt_jdx_line_addr,
                                line_size, this->channel,
                                spv_jdx_fetch_fin(virt_jdx_line_addr)))
                            {
                                spvSentJdxReq = true;
                            }
                            return false;
                        } else {
                            if (xc->readLAScratchTiming(virt_jdx_line_addr,
                                line_size, this->channel,
                                spv_jdx_fetch_fin(virt_jdx_line_addr)))
                            {
                                spvSentJdxReq = true;
                            }
                            return false;
                        }
                    } else {
                        //we already have a request for jdx line in flight
                        return false;
                    }
                }
            }
            //need to return zero here, since there is no element at jdx index
            if (virt_jdx < jdxVal || jdxOffset >= idx_elems) {
                uint8_t zeros;
                if (virt_jdx < jdxVal) {
                  //this is arbitrary...
                  zeros = std::min((unsigned)64,//(line_size/SIZE),
                                   (unsigned)(jdxVal-virt_jdx));
                } else {
                  zeros = std::min((unsigned)64,//(line_size/SIZE),
                                   (unsigned)(jdx_cnt - virt_jdx));
                }

/*
                ++spvIndex;
                this->done = (this->spvIndex == count);
                if (reg->dp()){
                    double d = 0.0;
                    data_fin((uint8_t *)&d, SIZE);
                } else {
                    float f = 0.0;
                    data_fin((uint8_t *)&f, SIZE);
                }

*/
                for (uint8_t z=0; z<zeros;++z) {
                  ++this->spvIndex;
                  this->done = (this->spvIndex == count);
                  if (reg->dp()){
                      double d = 0.0;
                      data_fin((uint8_t *)&d, SIZE);
                  } else {
                      float f = 0.0;
                      data_fin((uint8_t *)&f, SIZE);
                  }
                }

                if (virt_jdx-1+zeros == jdx_cnt-1){
                //if (virt_jdx == jdx_cnt-1){
                    //hit end of jdx row/col,
                    gotIdxValCur = false;
                    gotIdxValNxt = false;
                    jdxOffset = 0;
                }
                gotJdxVal    = false;
                gotDataVal   = false;
                return true;
            } 
            //------------------------------------------------------------------
            // dataValCur
            //------------------------------------------------------------------
            LAAddr data_addr = data_ptr + (idxValCur+jdxOffset)*SIZE;
            LAAddr data_line_offset = (data_addr%line_size);
            LAAddr data_line_addr = data_addr - data_line_offset;

            if(!gotDataVal) {
                if (spvCachedDataLineAddr == data_line_addr) {
                    dataVal = (uint8_t*)malloc(SIZE);
                    memcpy(dataVal,
                        (spvCachedDataLine+data_line_offset), SIZE);
                    gotDataVal = true;
                } else if(!spvSentDataReq) {
                    if (reg->loc() == LALoc::Mem) {
                        if(xc->readLAMemTiming(data_line_addr,
                            line_size, this->channel,
                            spv_data_fetch_fin(data_line_addr))) 
                        {
                            spvSentDataReq = true;
                        }
                        return false;
                    } else {
                        if(xc->readLAScratchTiming(data_line_addr,
                            line_size, this->channel,
                            spv_data_fetch_fin(data_line_addr))) 
                        {
                            spvSentDataReq = true;
                        }
                        return false;
                    }
                } else {
                    //we already have a request for data line in flight
                    return false;
                }
            }
            ++spvIndex;
            ++jdxOffset;
            this->done = (this->spvIndex == count);

            data_fin(dataVal, SIZE);
            free(dataVal);

            if(virt_jdx == jdx_cnt-1){
                //hit end of jdx row/col,
                gotIdxValCur = false;
                gotIdxValNxt = false;
                jdxOffset = 0;
            }
            gotJdxVal    = false;
            gotDataVal   = false;
            return true;
        };
    }

    //kick it off
    if (readFunction() && done) {
        done = false;
    } else {
        start();
    }
}


//=============================================================================
// LAMemUnitReadTimingParams
//=============================================================================

LAMemUnitReadTiming *
LAMemUnitReadTimingParams::create()
{
    return new LAMemUnitReadTiming(this);
}
