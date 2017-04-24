#!/usr/bin/env python

import os
import sys
from subprocess import call

dir_path = os.path.dirname(os.path.realpath(__file__))

if(len(sys.argv) != 1):
    print("usage: %s" % sys.argv[0])
    sys.exit(0)

#maps matrix size to how much memory we need
memsize_map = {
    '32': "512M",
    '64': "1G",
    '128': "4G",
    '256': '32G',
    '512': '64G',
    '1024': '128G',
    '2048': '128G'
}

#maps matrix size to how much time we need
time_map = {
    '32': "00:05:00",
    '64': "00:10:00",
    '128': "00:30:00",
    '256': "01:00:00",
    '512': "10:00:00",
    '1024': "30:00:00",
    '2048': "48:00:00"
}

###########################################################################
# SBATCH INVARINT and VARIANT ARGS
###########################################################################

SBATCH_ARGS = ["sbatch", "-n", "1", "-p", "batch"]

def mk_sbatchmem(params):
    return [("--mem=" + memsize_map[params["size"]])]

def mk_time(params):
    return ["-t", time_map[params["size"]]]

def mk_unique_id(params):
    return "_".join([
        #workload args
        "log_size=" + params["log_size"]
    ])

def mk_filename(unique_id):
    return ["-o", "output/" + unique_id]

def mk_jobname(unique_id):
    return ["-J", unique_id]


def mk_sbatch_args(params):
    unique_id = mk_unique_id(params)
    return SBATCH_ARGS + mk_sbatchmem(params) + mk_time(params) + \
        mk_filename(unique_id) + mk_jobname(unique_id)

###########################################################################
# GEM5 INVARIANT and VARIANT ARGS
###########################################################################

GEM5_ARGS = [(dir_path + "/../../build/X86/gem5.opt"),
             (dir_path + "/../../configs/thesis/x86_O3.py")]

workload = (dir_path + "/../../../linalg-benchmarks/" +
                            "benchmarks/out/dlu_solve_x86_sweep")

def mk_workload(params):
    return ["--cmd=" + workload + " --log_size=" + params["log_size"]]

def mk_gem5mem(params):
    return [("--mem_size=" + memsize_map[params["size"]] + "B")]

def mk_gem5_args(args, params):
    return GEM5_ARGS + mk_workload(params) + args + mk_gem5mem(params)

###########################################################################
# Generate command line for SBATCH
###########################################################################

#runner is just a proxy for the gem5 script
RUNNER = [(dir_path + "/runner.sh")]

def call_sbatch(args, params):
    call(mk_sbatch_args(params) + RUNNER + mk_gem5_args(args, params))

###########################################################################
# Build the different parameter configurations
###########################################################################

def workload_sweep(args, params):
    for log_size in [9,10]: #[5,6,7,8,9,10]
        params["size"] = str(1 << log_size)
        params["log_size"] = str(log_size)

        call_sbatch(args, params);

###########################################################################
# START PARAMETER SWEEP
###########################################################################
args = []
params = {}
workload_sweep(args, params)

