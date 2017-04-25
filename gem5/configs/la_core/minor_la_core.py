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
import math

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
ps.add_option('--cache_line_size',  type="int", default=64,
                                    help="System Cache Line Size in Bytes")
ps.add_option('--l1i_size',         type="string", default='16kB',
                                    help="L1 instruction cache size")
ps.add_option('--l1d_size',         type="string", default='64kB',
                                    help="L1 data cache size")
ps.add_option('--l2_size',          type="string", default='256kB',
                                    help="Unified L2 cache size")
ps.add_option('--mem_size',         type="string", default='2048MB',
                                    help="Size of the DRAM")
ps.add_option('--cpu_clk',          type="string", default='3GHz',
                                    help="Speed of all CPUs")

# LACORE SCRATCHPAD OPTIONS
ps.add_option('--scratch_size',       type="string", default='64kB',
                                      help="LACore Scratchpad size")
ps.add_option('--scratch_line_size',  type="string", default=64, 
                                      help="Scratchpad line size")

# LACORE CACHE OPTIONS
ps.add_option('--la_cache_size',    type="string", default='64kB',
                                    help="CPU LAExecCache size")
ps.add_option('--la_cache_banks',   type="int", default=4,
                                    help="number of LACache banks")
ps.add_option('--la_cache_ported',  type="int", default=True,
                                    help="If the mem-readers are multi-port")
ps.add_option('--la_use_dcache',    type="int", default=False,
                                    help="If D$ should be used instead of LA$")
ps.add_option('--la_use_l2cache',   type="int", default=False,
                                    help="If LACache banks should use L2$")

# LACORE EXECUNIT OPTIONS
ps.add_option('--la_exec_clk',      type="string", default='1GHz',
                                    help="Speed of all CPUs")
ps.add_option('--la_vec_nodes',     type="int", default=8,
                                    help="Num of parallel vecNodes in execUnit")
ps.add_option('--la_simd_width',    type="int", default=8,
                                    help="64-bit SIMD width in execUnit")

(options, args) = ps.parse_args()


###############################################################################
# LACore special cache hierarchy configuration setup
###############################################################################

multiport         = (options.la_cache_ported > 0)
mem_unit_channels = (4 if multiport else 1)

use_dcache        = (options.la_use_dcache > 0)
use_l2cache       = (options.la_use_l2cache > 0)
if(use_dcache and use_l2cache):
    print "cannot specify la_use_dcache and la_use_l2cache"
    sys.exit(0)

###############################################################################
# Setup System
###############################################################################

# create the system we are going to simulate
system = System(
    cache_line_size = options.cache_line_size,
    clk_domain = SrcClockDomain(
        clock = options.cpu_clk,
        voltage_domain = VoltageDomain()
    ),
    mem_mode = 'timing',
    mem_ranges = [AddrRange(options.mem_size)]
)

###############################################################################
# Create CPU and add simple Icache and Dcache
###############################################################################

# Create the LaCore CPU
system.cpu = MinorLACoreCPU(mem_unit_channels = mem_unit_channels)

# Create an L1 instruction and data cache

#latencies taken from here (Nehalem section)
#   http://dl.acm.org/citation.cfm?id=1669165
#   NOTE: i use 1 instead of 4 cycles bc Nehalem runs at 4 GHz, not 1 GHz
#         and assuming mem subsystem latencies don't change with clock
#         frequency, this will cut the latencies to 1/4
#assoc taken from CPU design (Thimmannagari) page 17
#mshrs taken from Kroft (original author):
#   http://dl.acm.org/citation.cfm?id=801868
system.cpu.icache = Cache(
    size = options.l1i_size,
    assoc = 4,
    tag_latency = 1,
    data_latency = 1,
    response_latency = 0,
    mshrs = 4,
    tgts_per_mshr = 20
)
system.cpu.dcache = Cache(
    size = options.l1d_size,
    assoc = 4,
    tag_latency = 1,
    data_latency = 1,
    response_latency = 0,
    mshrs = 4,
    tgts_per_mshr = 20
)

###############################################################################
# Create the Exec Unit
###############################################################################

#Dual Precision Divide (12 cycles for F32, 19 cycles for F64)
#  http://ieeexplore.ieee.org/document/7590039/
#Dual Precision Multiply (4 cycles)
#  http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=7314418
#Dual Precision Floating Point Add latency (4 cycles)
#  http://www.sciencedirect.com/science/article/pii/S1383762108000908
#TODO: use more clever algorithm to do bypassing in the vec node if a term is
#      zero or multiplying or dividing by 1. simple 1 cycle muxing should do
#TODO: use more accurate latencies for division/multiplication for sp/dp ops
system.cpu.laExecUnit = LACoreExecUnit(
    cfgOpUnit = LACoreCfgOpUnit(),
    xferOpUnit = LACoreXferOpUnit(
        memReader = LAMemUnitReadTiming(
            channel = (0 if multiport else 0),
            cacheLineSize = options.cache_line_size,
            scratchLineSize = options.scratch_line_size
        ),
        memWriter = LAMemUnitWriteTiming(
            channel = (1 if multiport else 0),
            cacheLineSize = options.cache_line_size,
            scratchLineSize = options.scratch_line_size
        )
    ),
    dataOpUnit = LACoreDataOpUnitTiming(
        srcAReader = LAMemUnitReadTiming(
            channel = (0 if multiport else 0),
            cacheLineSize = options.cache_line_size,
            scratchLineSize = options.scratch_line_size
        ),
        srcBReader = LAMemUnitReadTiming(
            channel = (1 if multiport else 0),
            cacheLineSize = options.cache_line_size,
            scratchLineSize = options.scratch_line_size
        ),
        srcCReader = LAMemUnitReadTiming(
            channel = (2 if multiport else 0),
            cacheLineSize = options.cache_line_size,
            scratchLineSize = options.scratch_line_size
        ),
        dstWriter = LAMemUnitWriteTiming(
            channel = (3 if multiport else 0),
            cacheLineSize = options.cache_line_size,
            scratchLineSize = options.scratch_line_size
        ),
        dataPath = LACoreDataOpUnitTimingDatapath(
            addLatencySp = 5,
            addLatencyDp = 5,
            mulLatencySp = 4,
            mulLatencyDp = 4,
            divLatencySp = 11,
            divLatencyDp = 15,
            simdWidthDp = options.la_simd_width,
            vecNodes = options.la_vec_nodes,
            clk_domain = SrcClockDomain(
                clock = options.la_exec_clk,
                voltage_domain = VoltageDomain()
            )
        )
    )
)

###############################################################################
# Create the Scratchpad
###############################################################################

system.cpu.scratchpad = LACoreScratchpad(
    size = options.scratch_size,
    lineSize = options.scratch_line_size,
    numPorts = mem_unit_channels,
    accessLatency = 1
)

###############################################################################
# Create l2Xbar and SystemXBar
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
system.cpu.laCache = LACoreLACache(
    addrRange = system.mem_ranges[0],
    cacheSize = options.la_cache_size,
    numBanks = options.la_cache_banks,
    numPorts = mem_unit_channels
)

# the laCache.xbar is used for 2 mututall-exclusive situations:
#   1) The LAMemUnits connect to the LACacheBanks
#   2) The LAMemUnits AND CPU connect to the Dcache
system.cpu.laCache.xbar = NoncoherentXBar(
    forward_latency = 0,
    frontend_latency = 0,
    response_latency = 0,
    width = options.cache_line_size
)

#-------------------------------------------------------------------------------

#creates the LACache banks and connects them and the ports to LACache's xbar
def createLACacheBanks(cpu):
    def isPowerOf2(num):
        return num != 0 and ((num & (num - 1)) == 0)

    if(not isPowerOf2(cpu.laCache.numBanks.value)):
        print "Error: numBanks is not a power of 2"
        sys.exit(0)

    if(not isPowerOf2(system.cache_line_size.value)):
        print "Error: cache_line_size is not a power of 2"
        sys.exit(0)

    log_banks = int(math.log(cpu.laCache.numBanks.value + 0.1, 2))
    log_line = int(math.log(options.cache_line_size, 2))
    bank_size = int(cpu.laCache.cacheSize.value / cpu.laCache.numBanks.value)

    for bank_id in range(0, cpu.laCache.numBanks.value):
        #create bank with interleaved address range and add to cache-list
        cache = Cache(
            size = str(bank_size) + "B",
            assoc = 4,
            tag_latency = 1,
            data_latency = 1,
            response_latency = 0,
            addr_ranges = [
                AddrRange(0, end = cpu.laCache.addrRange.end.value,
                    intlvHighBit = (log_line + log_banks),
                    intlvBits = log_banks,
                    intlvMatch = bank_id
                )
            ],
            mshrs = 4,
            tgts_per_mshr = 20
        )
        cpu.laCache.caches.append(cache)

#-------------------------------------------------------------------------------

if(not use_dcache):
    createLACacheBanks(system.cpu)

###############################################################################
# Connect CPU Ports
###############################################################################

def connectCPUPorts(cpu, l2bus, membus):
    #connect laCachePort
    cpu.la_cache_port = cpu.laCache.la_core_side

    #connect scratchpad ports for each mem_unit
    for channel in range(0, mem_unit_channels):
        cpu.scratch_port = cpu.scratchpad.port

    #connect all the l2xbar facing ports
    cpu.icache.mem_side = l2bus.slave
    cpu.dcache.mem_side = l2bus.slave
    cpu.icache_port = cpu.icache.cpu_side

    if(use_dcache):
        #use the laCache xbar between laMemUnits+cpu <--> dcache then
        cpu.dcache_port = cpu.laCache.xbar.slave
        cpu.laCache.xbar.master = cpu.dcache.cpu_side

        for port_id in range(0, cpu.laCache.numPorts):
            cpu.laCache.xbar_side = cpu.laCache.xbar.slave
    else:
        cpu.dcache_port = cpu.dcache.cpu_side

        #use the laCache xbar between laMemUnits <--> laCacheBanks then
        for bank_id in range(0, cpu.laCache.numBanks):
            cpu.laCache.xbar.master = cpu.laCache.caches[bank_id].cpu_side
        for port_id in range(0, cpu.laCache.numPorts):
            cpu.laCache.xbar_side = cpu.laCache.xbar.slave

        #we ARE using the LACache banks: are they connected to l2 or membus?
        for bank_id in range(0, cpu.laCache.numBanks):
            if(options.la_use_l2cache):
                cpu.laCache.caches[bank_id].mem_side = l2bus.slave
            else:
                cpu.laCache.caches[bank_id].mem_side = membus.slave

#-------------------------------------------------------------------------------

connectCPUPorts(system.cpu, system.l2bus, system.membus)

###############################################################################
# Everything from the l2Xbar down
###############################################################################

# Create an L2 cache and connect it to the l2bus and systemBus

#latencies taken from here (Nehalem section)
#   http://dl.acm.org/citation.cfm?id=1669165
#   NOTE: i use 1 instead of 4 cycles bc Nehalem runs at 4 GHz, not 1 GHz
#         and assuming mem subsystem latencies don't change with clock
#         frequency, this will cut the latencies to 1/4
system.l2cache = Cache(
    size = options.l2_size,
    assoc = 8,
    tag_latency = 4,
    data_latency = 4,
    response_latency = 0,
    mshrs = 20,
    tgts_per_mshr = 12
)

system.l2cache.mem_side = system.membus.slave
system.l2cache.cpu_side = system.l2bus.master

#create interrupt controller
system.cpu.createInterruptController()

# Connect the system up to the membus
system.system_port = system.membus.slave

# Create a DDR3 memory controller (wtf is ddr5 and hbm slower than ddr3?)
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
