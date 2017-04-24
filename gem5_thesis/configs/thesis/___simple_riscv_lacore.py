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

import m5
from m5.objects import *

# This config files sets up an LaCoreMinorCPU running RISCV ISA. It does 
# not attempt to support multiple workloads or multithreaded workloads or
# multiple cores. Additionally, it runs the LaCoreMinorCPU as a CPU, not 
# as a discrete accelerator to a host CPU. This will be done in other 
# scripts. Here we are simply stressing the new execution unit extension
# and various configurations of it


#why using another l1 cache instead of dcache and icache 
#1) can't use straight to L2 as that operates on cache_blocks 
#2) can't use straight to L2 as I need to implement flow control and snooping
#3) dont't want to use dcache since don't want to flood it with data used 
#   in the execution units only
#4) we want the execCache to be small and fast with a deep writebackqueue

parser = optparse.OptionParser()

parser.add_option("--cache-block-size", type="int", default=64, metavar="B",
                  help="Set Cache block size to B bytes")
parser.add_option("--sp-exec-units", type="int", default=1, metavar="S",
                  help="Set single precision unit count in the LaCore to S")
parser.add_option("--dp-exec-units", type="int", default=1, metavar="D",
                  help="Set double precision unit count in the LaCore to D")
parser.add_option("--scratch-size", type="string", default='64kB',
                  help="Set scratchpad size in bytes X")
parser.add_option("--sys-clock", action="store", type="string",
                  default='1GHz',
                  help = """Top-level clock for blocks running at system
                  speed""")

options for :
  simd_size
  vec_node_count
  scratchpad size     ! make sure the scratch_cache writes BLOCKS or WORDS???
  scratchpad latency 
  clock speed
  l1 dcache size (and other parameters that should vary)
  l1 icache size (and other parameters that should vary)

  l2 cache size (and other parameters that should vary)

  memory parameters that should vary 

  nuber of sp units 
  number of dp units 

  workload 

  cache block size

system = System(cache_line_size = block_size)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.cpu = MinorCPU()

system.memobj = SimpleMemobj()

system.cpu.icache_port = system.memobj.inst_port
system.cpu.dcache_port = system.memobj.data_port

system.membus = SystemXBar()

system.memobj.mem_side = system.membus.slave

system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.master
system.cpu.interrupts[0].int_master = system.membus.slave
system.cpu.interrupts[0].int_slave = system.membus.master

system.mem_ctrl = DDR3_1600_x64()
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master

system.system_port = system.membus.slave

process = LiveProcess()
process.cmd = ['tests/test-progs/hello/bin/x86/linux/hello']
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system = False, system = system)
m5.instantiate()

block_size = 64

system.cpu = LaCoreMinorCPU()

system.cpu.scratchpad = LaCoreScratchpad()

system.cpu.scratchpad.cache_size = '2kB'

system.cpu.scratchpad.cache = LaCoreScratchpadCache(size = '32kB', assoc = 4,
                 tag_latency = 1, data_latency = 1, response_latency = 1,
                 tgts_per_mshr = 8, clusivity = 'mostly_incl',
                 writeback_clean = True)

system.cpu.scratchpad = LaCoreScratchpad(
                          size = "64kB",
                          bankAccessLatency = 1,
                          numBanks = 32,
                          bytesPerBankAccess = 4,
                          readBankAccessesPerCycle = 1,
                          writeBankAccessesPerCycle = 1)

system.cpu.scratchpad.mem_port = system.memory

          xbar = L2XBar()

system.cpu.icache_port = system.cache.cpu_side
system.cpu.dcache_port = system.cache.cpu_side
system.cpu.scratchpad_port = system.cpu.scratchpad.cpu_side

if options.blocking:
     proto_l1.mshrs = 1
else:
     proto_l1.mshrs = 4

cache_proto = [proto_l1]

# Connect the lowest level crossbar to the memory
last_subsys = getattr(system, 'l%dsubsys0' % len(cachespec))
last_subsys.xbar.master = system.physmem.port
last_subsys.xbar.point_of_coherency = True

# Now add additional cache levels (if any) by scaling L1 params, the
# first element is Ln, and the last element L1
for scale in cachespec[:-1]:
     # Clone previous level and update params
     prev = cache_proto[0]
     next = prev()
     next.size = prev.size * scale
     next.tag_latency = prev.tag_latency * 10
     next.data_latency = prev.data_latency * 10
     next.response_latency = prev.response_latency * 10
     next.assoc = prev.assoc * scale
     next.mshrs = prev.mshrs * scale

     # Swap the inclusivity/exclusivity at each level. L2 is mostly
     # exclusive with respect to L1, L3 mostly inclusive, L4 mostly
     # exclusive etc.
     next.writeback_clean = not prev.writeback_clean
     if (prev.clusivity.value == 'mostly_incl'):
          next.clusivity = 'mostly_excl'
     else:
          next.clusivity = 'mostly_incl'

     cache_proto.insert(0, next)


# The system port is never used in the tester so merely connect it
# to avoid problems
root.system.system_port = last_subsys.xbar.slave


print "Beginning simulation!"
exit_event = m5.simulate()
print 'Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause())
