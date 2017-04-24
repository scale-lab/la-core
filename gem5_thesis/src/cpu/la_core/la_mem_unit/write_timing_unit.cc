//=============================================================================
// LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#include "cpu/la_core/la_mem_unit/write_timing_unit.hh"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>

#include "base/types.hh"
#include "cpu/la_core/exec_context.hh"
#include "cpu/la_core/la_insn.hh"
#include "cpu/la_core/la_reg.hh"
#include "cpu/la_core/la_types.hh"
#include "debug/LAMemUnitWriteTiming.hh"
#include "params/LAMemUnitWriteTiming.hh"
#include "sim/ticked_object.hh"

//=============================================================================
// constructor/destructor
//=============================================================================

LAMemUnitWriteTiming::LAMemUnitWriteTiming(LAMemUnitWriteTimingParams *p) :
    TickedObject(p), channel(p->channel), cacheLineSize(p->cacheLineSize),
    scratchLineSize(p->scratchLineSize), done(false)
{
}

LAMemUnitWriteTiming::~LAMemUnitWriteTiming()
{
}

//=============================================================================
// Overrides
//=============================================================================

//make it a clocked object
void
LAMemUnitWriteTiming::evaluate()
{
    //Do NOT assert if done here! it coud've finished in < 1 cycle!
    assert(running);
    if (!done) { writeFunction(); }
    if (done) {
        done = false;
        stop();
    }
}

void
LAMemUnitWriteTiming::regStats()
{
  TickedObject::regStats();
  ClockedObject::regStats();
}

//=============================================================================
// main functions
//=============================================================================

void 
LAMemUnitWriteTiming::queueData(uint8_t *data)
{
    assert(running && !done);
    dataQ.push_back(data);
}

//to main mem will ALWAYS succeed. scratchpad will be bandwidth throttled
//either way, on writes, we must wait for responses!
//
// NOTE: delete[]s all data from dataQ if written to fn(), so the
//   exec_context must memcpy the data!
void
LAMemUnitWriteTiming::initialize(LACount count, LAReg * reg,
  LACoreExecContext * xc, std::function<void(bool)> on_item_store)
{
    assert(!running && !done);
    assert(count > 0);
    assert(!dataQ.size());

    uint8_t SIZE = reg->dp() ? sizeof(double) : sizeof(float);

    // This function tries to get up to 'get_up_to' elements from the front of
    // the input queue. if there are less elements, it returns all of them
    // if fn() is successfull, try_write DELETES the data therefore, fn()
    // or some downstream function needs to memcpy it!
    //
    // NOTE: be careful with uint8_t overflow here...
    auto try_write = [SIZE,this](uint8_t get_up_to,
        std::function<uint8_t(uint8_t *, uint8_t)> fn)
    {
        uint64_t can_get = this->dataQ.size();
        if (!can_get) {
            DPRINTF(LAMemUnitWriteTiming, "try_write dataQ empty\n");
            return false;
        }
        uint64_t got = std::min((uint64_t)get_up_to, can_get);
        uint8_t *buf = new uint8_t[got*SIZE];
        for (uint8_t i=0; i<got; ++i) {
            memcpy(buf+SIZE*i, this->dataQ[i], SIZE);
        }
        uint64_t actually_written;
        if ((actually_written = fn(buf, (uint8_t)got))) {
            delete [] buf;
            for (uint8_t i=0; i<actually_written; ++i) {
                uint8_t * data = dataQ.front();
                delete [] data;
                dataQ.pop_front();
            }
            return true;
        } else {
            DPRINTF(LAMemUnitWriteTiming, "fn(data) failed\n");
            return false;
        }
    };

    if (!reg->vector()) {
        assert(count == 1);
        LAAddr addr = reg->scalarAddr();
        //need 'this' for DPRINTF
        auto fin = [on_item_store,this](){
            DPRINTF(LAMemUnitWriteTiming, "calling on_item_store\n");
            on_item_store(true);
            DPRINTF(LAMemUnitWriteTiming, "called on_item_store\n");
        };
        switch(reg->loc()){
            case LALoc::Reg: 
                writeFunction = [try_write,reg,fin,this](){
                    return try_write(1,
                        [reg,fin,this](uint8_t *data, uint8_t cnt)
                    {
                        assert(cnt == 1);
                        if (reg->dp()){
                            reg->scalarDouble(*(double *)data);
                        } else {
                            reg->scalarFloat(*(float *)data);
                        }
                        fin();
                        this->done = true;
                        return 1;
                    });
                };
                break;
            case LALoc::Mem: 
                writeFunction = [try_write,addr,SIZE,fin,xc,this](){
                    return try_write(1,
                        [addr,SIZE,fin,xc,this](uint8_t *data, uint8_t cnt)
                    {
                        assert(cnt == 1);
                        this->done = xc->writeLAMemTiming(addr, data, SIZE,
                            channel, fin);
                        return 1;
                    });
                };
                break;
            case LALoc::Sch:
                writeFunction = [try_write,addr,SIZE,fin,xc,this](){
                    return try_write(1,
                        [addr,SIZE,fin,xc,this](uint8_t *data, uint8_t cnt)
                    {
                        assert(cnt == 1);
                        this->done = xc->writeLAScratchTiming(addr, data, SIZE,
                            channel, fin);
                        return 1;
                    });
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

        //need 'this' for DPRINTF
        auto fin = [on_item_store,count,this](LACount i, LACount items_ready) {
            return [on_item_store,count,i,items_ready,this]() {
                for (uint64_t j=0; j<items_ready; ++j) {
                    bool _done = ( (i+j+1) == count );
                    DPRINTF(LAMemUnitWriteTiming,
                        "calling on_item_store with 'done'=%d\n", _done);
                    on_item_store(_done);
                }
            };
        };

        writeFunction = [try_write,reg,fin,xc,vaddr,vstride,vcount,
            on_item_store,vskip,SIZE,count,this](void) ->bool
        {
            LACount i = this->vecIndex;
            LAAddr addr = vaddr + SIZE*(vstride*i + vskip*(i/vcount));

            //scratch and cache could use different line sizes
            uint64_t line_size;
            switch (reg->loc()) {
                case LALoc::Mem: line_size = cacheLineSize; break;
                case LALoc::Sch: line_size = scratchLineSize; break;
                default: panic("invalid reg->loc()"); break;
            }

            //we can always write 1 element
            LAAddr line_addr = addr - (addr % line_size);
            uint64_t consec_items = 1;

            //now find any consecutive items that we can also write
            for (uint64_t j=1; j<(line_size/SIZE) && (i+j)<count; ++j) {
                LAAddr next_addr = vaddr + 
                    SIZE*((i+j)*vstride + vskip*((i+j)/vcount));
                LAAddr next_line_addr = next_addr - (next_addr % line_size);
                if (next_addr-SIZE*j == addr && line_addr == next_line_addr) {
                    ++consec_items;
                } else {
                    break;
                }
            }

            //now see if there is any data in the queue to write
            DPRINTF(LAMemUnitWriteTiming,
                "getting data to write %d items at %#x\n", consec_items, addr);
            return try_write(consec_items, [fin,reg,xc,addr,SIZE,
                count,i,this](uint8_t * data, uint8_t items_ready) -> uint8_t 
            {
                DPRINTF(LAMemUnitWriteTiming, "got %d items to write at %#x\n",
                    items_ready, addr);
                
                bool success;
                if (reg->loc() == LALoc::Mem) {
                    success = xc->writeLAMemTiming(addr, data, 
                        SIZE*items_ready, this->channel, fin(i, items_ready));
                } else {
                    success = xc->writeLAScratchTiming(addr, data, 
                        SIZE*items_ready, this->channel, fin(i, items_ready));
                }
                if (success) {
                    this->vecIndex += items_ready;
                    this->done = (this->vecIndex == count);
                    return items_ready;
                }
                return 0;
            });
        };
    } else {
        //TODO: sparse (NOT YET IMPLEMENTED)
        assert(false);
    }

    //kick it off
    if (writeFunction() && done) {
        done = false;
    } else {
        start();
    }
}

//=============================================================================
// LAMemUnitWriteTimingParams
//=============================================================================

LAMemUnitWriteTiming *
LAMemUnitWriteTimingParams::create()
{
    return new LAMemUnitWriteTiming(this);
}
