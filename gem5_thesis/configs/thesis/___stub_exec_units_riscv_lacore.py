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
# This config file sets up the bare minimum environment for a single LaCore
# with 
###############################################################################

import sys
import m5
from m5.objects import *

from optparse import OptionParser

from AddrRange import AddrRange
from Cache import Cache
from DDR3_1600_x64 import DDR3_1600_x64
from L2XBar import L2XBar
from LiveProcess import LiveProcess
from SrcClockDomain import SrcClockDomain
from System import System
from SystemXBar import SystemXBar
from VoltageDomain import VoltageDomain

from LaCoreCPU import *
from LaCoreExecUnit import *

from LaCoreOpUnit import *
from LaCoreCfgOpUnit import *
from LaCoreCfgrOpUnit import *
from LaCoreDataOpUnit import *
from LaCoreXferOpUnit import *

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
ps.add_option('--mem_size',         type="string", default='512MB',
                                    help="Size of the DRAM")
ps.add_option('--clk_freq',         type="string", default='1GHz',
                                    help="Speed of all CPUs")

# LACORE OPTIONS
ps.add_option('--queue_size',       type="int", default=8,
                                    help="Queue size of all ExecUnit queues")
ps.add_option('--simd_width',       type="int", default=8,
                                    help="SIMD width of the ExecUnits")
ps.add_option('--vec_node_count',   type="int", default=8,
                                    help="Number of VecNodes in an ExecUnit")
ps.add_option('--num_sp_units',     type="int", default=1,
                                    help="Number of SP-float Units per cpu")
ps.add_option('--num_dp_units',     type="int", default=1,
                                    help="Number of DP-float Units per cpu")
ps.add_option('--exec_cache_size',  type="string", default='16kB', 
                                    help="L1 exec cache size")
ps.add_option('--scratch_size',     type="string", default='64kB', 
                                    help="L1 exec cache size")

(options, args) = ps.parse_args()

###############################################################################
# Create Latencies 
###############################################################################

def latencies(float_dp):
  if(float_dp):
    return {
      'cfg_op_latency': 1,
      'cfgr_op_latency': 1,
      'add_latency': 3,
      'subtract_latency': 3,
      'multiply_latency': 3,
      'divide_latency': 5,
      'modulo_latency': 5,

      'data_op_debug_latency': 1000,
      'xfer_op_debug_latency': 500
    }
  else:
    return {
      'cfg_op_latency': 1,
      'cfgr_op_latency': 1,
      'add_latency': 5,
      'subtract_latency': 5,
      'multiply_latency': 5,
      'divide_latency': 9,
      'modulo_latency': 9,

      'data_op_debug_latency': 1000,
      'xfer_op_debug_latency': 500
    }

###############################################################################
# Setup System 
###############################################################################

# create the system we are going to simulate
system = System(cache_line_size = options.cache_line_size)

# Set the clock fequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = options.clk_freq
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = 'timing'                        # Use timing accesses
system.mem_ranges = [AddrRange(options.mem_size)] # Create an address range

###############################################################################
# Create CPU and add simple Icache and Dcache
###############################################################################

# Create the LaCore CPU
system.cpu = LaCoreCPU()

# Create an L1 instruction and data cache
system.cpu.icache = Cache(size = options.l1i_size, 
                          assoc = 2,
                          tag_latency = 2, 
                          data_latency = 2, 
                          response_latency = 2
                          mshrs = 4
                          tgts_per_mshr = 20)

system.cpu.dcache = Cache(size = options.l1d_size
                          assoc = 2,
                          tag_latency = 2, 
                          data_latency = 2, 
                          response_latency = 2
                          mshrs = 4
                          tgts_per_mshr = 20)

###############################################################################
# Add LaCore specific SimObjects to the CPU
###############################################################################

#system.cpu.token_ring = LaCoreTokenRing(size = options.num_dp_units + 
                                               #options.num_sp_units)
#
#system.cpu.scratchpad = LaCoreScratchpad(size = options.scratch_size,
                                         #bankAccessLatency = 1,
                                         #numBanks = 32,
                                         #bytesPerBankAccess = 4,
                                         #readBankAccessesPerCycle = 1,
                                         #writeBankAccessesPerCycle = 1)
#
#system.cpu.exec_cache_tlb = LaCoreExecCacheTLB(latency = 1)
#
#system.cpu.exec_cache = LaCoreExecCache(size = options.exec_cache_size, 
                                        #assoc = 4,
                                        #tag_latency = 1, 
                                        #data_latency = 1, 
                                        #response_latency = 1,
                                        #tgts_per_mshr = 8, 
                                        #clusivity = 'mostly_incl',
                                        #writeback_clean = True)
#
#system.cpu.mem_unit = LaCoreMemUnit(token_ring = system.cpu.token_ring,
                                    #scratchpad = system.cpu.scratchpad,
                                    #exec_cache_tlb = system.cpu.exec_cache_tlb,
                                    #exec_cache = system.cpu.exec_cache,
                                    #queue_size = options.queue_size)

system.cpu.sp_units = []
for i in xrange(options.num_sp_units):
    exec_unit = LaCoreExecUnit(is_double = False,
                               cpu_index = 0,
                               exec_index = i,
                               #mem_unit = system.cpu.mem_unit,
                               #simd_width = options.simd_width,
                               #vec_node_count = options.vec_node_count,
                               latencies = latencies)
    system.cpu.sp_units.append(exec_unit)

system.cpu.dp_units = []
for i in xrange(options.num_sp_units):
    exec_unit = LaCoreExecUnit(is_double = True,
                               cpu_index = 0,
                               exec_index = i,
                               #mem_unit = system.cpu.mem_unit,
                               #simd_width = options.simd_width,
                               #vec_node_count = options.vec_node_count,
                               latencies = latencies)
    system.cpu.dp_units.append(exec_unit)

###############################################################################
# Create l2Xbar and connect all of the CPU's ports
###############################################################################

# Create a memory bus, a coherent crossbar, in this case
system.l2bus = L2XBar()

#connect all the cpu facing ports
#system.cpu.mem_unit.scratchpad.port = system.cpu.mem_unit.scratchpad_port
#system.cpu.mem_unit.exec_cache.cpu_side = system.cpu.mem_unit.exec_cache_port
#system.cpu.mem_unit.exec_cache.mem_side = l2bus.slave

#connect all the l2xbar facing ports
system.cpu.icache.mem_side = l2bus.slave
system.cpu.dcache.mem_side = l2bus.slave
system.cpu.icache.cpu_side = system.cpu.icache_port
system.cpu.dcache.cpu_side = system.cpu.dcache_port

###############################################################################
# Everything from the l2Xbar down
###############################################################################

# Create a memory bus
system.membus = SystemXBar()

# Create an L2 cache and connect it to the l2bus and systemBus
system.l2cache = Cache( size = options.l2_size,
                        assoc = 8,
                        tag_latency = 20,
                        data_latency = 20,
                        response_latency = 20,
                        mshrs = 20,
                        tgts_per_mshr = 12)

system.l2cache.mem_side = l2bus.master
system.l2cache.cpu_side = membus.slave

#create interrupt controller
system.cpu.createInterruptController()

# Connect the system up to the membus
system.system_port = system.membus.slave

# Create a DDR3 memory controller
system.mem_ctrl = DDR3_1600_x64()
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
    process.cmd = options.cmd

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
