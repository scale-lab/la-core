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

#from AtomicLACoreSimpleCPU import AtomicLACoreSimpleCPU

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

# LACORE OPTIONS
ps.add_option('--scratch_size',     type="string", default='64kB', 
                                    help="Scratchpad size")

(options, args) = ps.parse_args()

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
    mem_mode = 'atomic',
    mem_ranges = [AddrRange(options.mem_size)]
)

###############################################################################
# Create CPU and add simple Icache and Dcache
###############################################################################

# Create the LaCore CPU
system.cpu = AtomicLACoreSimpleCPU(scratchSize = options.scratch_size)

# Create an L1 instruction and data cache
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
# Create l2Xbar and connect all of the CPU's ports
###############################################################################

# Create a memory bus, a coherent crossbar, in this case
system.l2bus = L2XBar()

#connect all the l2xbar facing ports
system.cpu.icache.mem_side = system.l2bus.slave
system.cpu.dcache.mem_side = system.l2bus.slave
system.cpu.icache.cpu_side = system.cpu.icache_port
system.cpu.dcache.cpu_side = system.cpu.dcache_port

###############################################################################
# Everything from the l2Xbar down
###############################################################################

# Create a memory bus
system.membus = SystemXBar()

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
    print "No --cmd=<workload> passed in"
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
