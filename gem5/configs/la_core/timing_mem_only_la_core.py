# -*- coding: utf-8 -*-
# Copyright (c) 2017 Brown University
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Samuel Steffl


###############################################################################
# The point of this configuration is to run functional tests on the RISC-V
# decoder as well as the support infrastructure for the LACore. After all the
# tests are passing, you should probably run the timing mode simulation
###############################################################################

import sys
import m5
from m5.objects import *
from m5.util import addToPath, fatal
from optparse import OptionParser

addToPath('../')

from common import Options
from common import Simulation
from common import CacheConfig
from common import CpuConfig
from common import MemConfig
from common.Caches import *

###############################################################################
# Parse options 
###############################################################################

ps = OptionParser()

# GENERAL OPTIONS
ps.add_option('--cmd',              type="string",
                                    help="Command to run on the CPU")
ps.add_option('--cache_line_size',  type="int", default=128,
                                    help="System Cache Line Size in Bytes")
ps.add_option('--l1i_size',         type="string", default='32kB',
                                    help="L1 instruction cache size")
ps.add_option('--l1d_size',         type="string", default='32kB',
                                    help="L1 data cache size")
ps.add_option('--l2_size',          type="string", default='128kB',
                                    help="Unified L2 cache size")
ps.add_option('--mem_size',         type="string", default='512MB',
                                    help="Size of the DRAM")
ps.add_option('--cpu_clk',          type="string", default='3GHz',
                                    help="Speed of all CPUs")

# LACORE OPTIONS
ps.add_option('--scratch_size',     type="string", default='256kB', 
                                    help="CPU Scratchpad size")
ps.add_option('--la_cache_size',    type="string", default='128kB',
                                    help="CPU LAExecCache size")
ps.add_option('--la_cache_banks',   type="string", default=8,
                                    help="Number of banks in the LACache")
ps.add_option('--la_cache_ported',  type="int", default=True,
                                    help="Whether the LACache is mult-ported")

(options, args) = ps.parse_args()

###############################################################################
# Setup System 
###############################################################################

# create the system we are going to simulate
system = System(cache_line_size = options.cache_line_size)

# Set the clock fequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = options.cpu_clk
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = 'timing'                        # Use timing accesses
system.mem_ranges = [AddrRange(options.mem_size)] # Create an address range

###############################################################################
# Create CPU and add simple Icache and Dcache
###############################################################################

# Create the LaCore CPU
system.cpu = TimingLACoreSimpleCPU()

# Create an L1 instruction and data cache

#latencies taken from here (Nehalem section)
#   http://dl.acm.org/citation.cfm?id=1669165
#   NOTE: i use 1 instead of 4 cycles bc Nehalem runs at 4 GHz, not 1 GHz
#         and assuming mem subsystem latencies don't change with clock
#         frequency, this will cut the latencies to 1/4
#assoc taken from CPU design (Thimmannagari) page 17
#mshrs taken from Kroft (original author):
#   http://dl.acm.org/citation.cfm?id=801868
system.cpu.icache = Cache(size = options.l1i_size,
                          assoc = 4,
                          tag_latency = 1,
                          data_latency = 1,
                          response_latency = 0,
                          mshrs = 4,
                          tgts_per_mshr = 20)

system.cpu.dcache = Cache(size = options.l1d_size,
                          assoc = 4,
                          tag_latency = 1,
                          data_latency = 1,
                          response_latency = 0,
                          mshrs = 4,
                          tgts_per_mshr = 20)

###############################################################################
# Create the Exec Unit
###############################################################################

channel_id = 4#0
def channel():
    global channel_id
    if(options.la_cache_ported):
        tmp_id = channel_id
        channel_id += 1
        return tmp_id
    else:
        return channel_id

dataOpUnit = LACoreDataOpUnitAtomic(
    srcAReader = LAMemUnitReadTiming(channel = 0),#channel()),
    srcBReader = LAMemUnitReadTiming(channel = 1),#channel()),
    srcCReader = LAMemUnitReadTiming(channel = 2),#channel()),
    dstWriter = LAMemUnitWriteTiming(channel = 3))#channel()))

cfgOpUnit = LACoreCfgOpUnit()

xferOpUnit = LACoreXferOpUnit(
    memReader = LAMemUnitReadTiming(channel = 0),#channel()),
    memWriter = LAMemUnitWriteTiming(channel = 1))#channel()))

system.cpu.laExecUnit = LACoreExecUnit(dataOpUnit = dataOpUnit,
                                       cfgOpUnit = cfgOpUnit,
                                       xferOpUnit = xferOpUnit)

system.cpu.mem_unit_channels = channel_id

###############################################################################
# Create the Scratchpad
###############################################################################

system.cpu.scratchpad = LACoreScratchpad(size = options.scratch_size,
                                         bankAccessLatency = 1,
                                         numBanks = 32,
                                         bytesPerBankAccess = 8,
                                         readBankAccessesPerCycle = 6,
                                         writeBankAccessesPerCycle = 2)


###############################################################################
# Create l2Xbar
###############################################################################

# Create a memory bus, a coherent crossbar, in this case
system.l2bus = L2XBar()

# Create a memory bus
system.membus = SystemXBar()

###############################################################################
# Create the LAExecCache and TLB
###############################################################################

#NOTE: LACacheTLB lookups are currently 0 cycle-latency due to a bug
system.cpu.laCacheTLB = LACoreLACacheTLB(latency = 1)

#latencies taken from here (Nehalem section)
#   http://dl.acm.org/citation.cfm?id=1669165
#   NOTE: i use 1 instead of 4 cycles bc Nehalem runs at 4 GHz, not 1 GHz
#         and assuming mem subsystem latencies don't change with clock
#         frequency, this will cut the latencies to 1/4
system.cpu.laCache = LACoreLACache(addrRange = system.mem_ranges[0],
                                   cacheSize = options.la_cache_size,
                                   #numBanks = options.la_cache_banks,
                                   numBanks = 1,
                                   numPorts = channel_id)
def isPowerOf2(num):
    return num != 0 and ((num & (num - 1)) == 0)

def createXbarPorts(self):
    for port_id in range(0, self.numPorts):
        #xbarPort = MasterPort("port facing the LACache xbar")
        #self.xbarPorts.append(xbarPort)
        self.xbar.slave = self.xbar_side
        #self.xbar_side = self.caches[port_id].cpu_side

def createCacheBanks(self, l2Xbar):
    self.caches = []

    if(not isPowerOf2(self.numBanks.value)):
        print "Error: numBanks is not a power of 2"
        import sys
        sys.exit(0)

    if(not isPowerOf2(system.cache_line_size.value)):
        print "Error: cache_line_size is not a power of 2"
        import sys
        sys.exit(0)

    import math
    log_banks = int(math.log(self.numBanks.value, 2))
    log_line = int(math.log(system.cache_line_size.value, 2))

    bank_size = int(self.cacheSize.value / self.numBanks.value)
    bank_range = int(self.addrRange.end.value / self.numBanks.value)
    for bank_id in range(0, 1):#self.numPorts):#numBanks):
        #addr_range = AddrRange(bank_range*bank_id, bank_range*(bank_id+1))
        addr_range = AddrRange(0, end = system.mem_ranges[0].end.value,
                                  intlvHighBit = (log_line + log_banks),
                                  intlvBits = log_banks,
                                  intlvMatch = bank_id )
        #cache = Cache(size = str(bank_size) + "B",
        cache = Cache(size = self.cacheSize,
                      assoc = 8,
                      tag_latency = 4,
                      data_latency = 4,
                      response_latency = 0,
                      #addr_ranges = [addr_range],
                      mshrs = 20,
                      tgts_per_mshr = 8)

        self.caches.append(cache)
        self.xbar.master = cache.cpu_side
        l2Xbar.slave = cache.mem_side

system.cpu.laCache.xbar = NoncoherentXBar(forward_latency = 0,
                                          frontend_latency = 0,
                                          response_latency = 0,
                                          width = options.cache_line_size)
createCacheBanks(system.cpu.laCache, system.membus)#system.l2bus)
createXbarPorts(system.cpu.laCache)


###############################################################################
# Connect all of the CPU's ports and L2XBar
###############################################################################

#connect all the l2xbar facing ports
system.cpu.icache.mem_side = system.l2bus.slave
system.cpu.dcache.mem_side = system.l2bus.slave
system.cpu.icache.cpu_side = system.cpu.icache_port
system.cpu.dcache.cpu_side = system.cpu.dcache_port

#connect the ports of the 2 custom LACore memory elements
system.cpu.laCache.la_core_side = system.cpu.la_cache_port
system.cpu.scratchpad.port = system.cpu.scratch_port

###############################################################################
# Everything from the l2Xbar down
###############################################################################

# Create an L2 cache and connect it to the l2bus and systemBus

#latencies taken from here (Nehalem section)
#   http://dl.acm.org/citation.cfm?id=1669165
#   NOTE: i use 1 instead of 4 cycles bc Nehalem runs at 4 GHz, not 1 GHz
#         and assuming mem subsystem latencies don't change with clock
#         frequency, this will cut the latencies to 1/4
system.l2cache = Cache( size = options.l2_size,
                        assoc = 8,
                        tag_latency = 4,
                        data_latency = 4,
                        response_latency = 0,
                        mshrs = 20,
                        tgts_per_mshr = 8)

system.l2cache.mem_side = system.membus.slave
system.l2cache.cpu_side = system.l2bus.master

#create interrupt controller
system.cpu.createInterruptController()

# Connect the system up to the membus
system.system_port = system.membus.slave

# Create a DDR3 memory controller
system.mem_ctrl = DDR3_1600_x64(device_size = options.mem_size)
#system.mem_ctrl = GDDR5_4000_x64(device_size = options.mem_size)
#system.mem_ctrl = HBM_1000_4H_x64(device_size = options.mem_size)
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master

###############################################################################
# Create Workload
###############################################################################

process = LiveProcess()

if not options or not options.cmd:
    print "No --cmd='<workload> [args...]' passed in"
    sys.exit(1)
else:
    split = options.cmd.split(' ')
    filtered = []
    for s in options.cmd.split(' '):
      if len(s):
        filtered = filtered + [s]

    process.executable = filtered[0]
    process.cmd = filtered

system.cpu.workload = process
system.cpu.createThreads()

###############################################################################
# Run Simulation
###############################################################################

# set up the root SimObject and start the simulation
root = Root(full_system = False, system = system)
# instantiate all of the objects we've created above
m5.instantiate()

print "Beginning simulation!"
exit_event = m5.simulate()
print 'Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause())
