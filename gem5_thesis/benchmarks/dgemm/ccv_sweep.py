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
    '1024': '128G'
}

#maps matrix size to how much time we need
time_map = {
    '32': "00:05:00",
    '64': "00:10:00",
    '128': "00:30:00",
    '256': "01:00:00",
    '512': "04:00:00",
    '1024': "08:00:00"
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
        "size=" + params["size"],
        "useSCH=" + params["use_scratch"],
        "usePNL=" + params["use_panel"],

        #exec unit args
        "vec=" + params["vec_nodes"],
        "simd=" + params["simd"],

        #scratch args
        "ssize=" + params["ssize_log"],
        "sline=" + params["scratch_line"],

        #cache hierarhcy args
        "line=" + params["cache_line"],
        "useL1=" + params["la_use_l1d"],
        "useL2=" + params["la_use_l2"],
        "dsize=" + params["l1d_size_log"],
        "l2size=" + params["l2_size_log"],
        "lasize=" + params["la_size_log"],
        "labank=" + params["la_banks"]
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

GEM5_ARGS = [(dir_path + "/../../build/RISCV_LA_CORE/gem5.opt"),
             (dir_path + "/../../configs/thesis/full_timing_la_core.py")]

workload = (dir_path + "/dgemm_la_core_sweep")

def mk_workload(params):
    return ["--cmd=" + workload + " --size=" + params["size"] +
            " --bs=" + params["size"] +
            ((" --scratch_size=" + params["ssize_log"])
                if (params["use_scratch"] == "YES")
                else " --no_scratch") +
            ("" if (params["use_panel"] == "YES") else " --no_panel")]

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

def exec_sweep(args, params):
    for simd_width in [4,8]: #[4, 8]:
        for vec_nodes in [8,16]: #[4, 8, 16]:
            params["vec_nodes"] = str(vec_nodes)
            params["simd"] = str(simd_width)
            call_sbatch(
                args + [
                    ("--la_vec_nodes=" + str(vec_nodes)),
                    ("--la_simd_width=" + str(simd_width))],
                params)

def dcache_sweep(args, params):
    for cache_line in [128]: #[32, 64, 128]:
        for l1d_size_log in [16]: #[15,16]:
            for l2_size_log in [18]: #[17,18,19]:
                params["cache_line"] = str(cache_line)
                params["l1d_size_log"] = str(l1d_size_log)
                params["l2_size_log"] = str(l2_size_log)
                params["la_use_l1d"] = "YES"
                params["la_use_l2"] = "NO"
                params["la_banks"] = str(0)
                params["la_size_log"] = str(0)
                exec_sweep(
                    args + [
                        ("--cache_line_size=" + str(cache_line)),
                        ("--l1d_size=" + str(1 << l1d_size_log) + "B"),
                        ("--l2_size=" + str(1 << l2_size_log) + "B"),
                        ("--la_use_dcache=1"),
                        ("--la_use_l2cache=0"),
                        ("--la_cache_banks=" + str(0)),
                        ("--la_cache_size=" + str(0) + "B")],
                    params)


def la_cache_sweep(args, params):
    for cache_line in [128]: #[32, 64, 128]:
        for l2_size_log in [18]: #[17,18]: #[17,18,19]:
            for banks in [1,2,4,8,16]: #[1,2,4,8,16]:

                # not using l2 underneath
                for la_size_log in []: #[17,18]: #[17,18,19]:
                    params["cache_line"] = str(cache_line)
                    params["l1d_size_log"] = str(0)
                    params["l2_size_log"] = str(l2_size_log)
                    params["la_use_l1d"] = "NO"
                    params["la_use_l2"] = "NO"
                    params["la_banks"] = str(banks)
                    params["la_size_log"] = str(la_size_log)
                    exec_sweep(
                        args + [
                            ("--cache_line_size=" + str(cache_line)),
                            ("--l2_size=" + str(1 << l2_size_log) + "B"),
                            ("--la_use_dcache=0"),
                            ("--la_use_l2cache=0"),
                            ("--la_cache_banks=" + str(banks)),
                            ("--la_cache_size=" + str(1 << la_size_log) + "B")
                        ],
                        params)

                # using l2 underneath
                for la_size_log in [16]: #[15,16,17]:
                    params["cache_line"] = str(cache_line)
                    params["l1d_size_log"] = str(0)
                    params["l2_size_log"] = str(l2_size_log)
                    params["la_use_l1d"] = "NO"
                    params["la_use_l2"] = "YES"
                    params["la_banks"] = str(banks)
                    params["la_size_log"] = str(la_size_log)
                    exec_sweep(
                        args + [
                            ("--cache_line_size=" + str(cache_line)),
                            ("--l2_size=" + str(1 << l2_size_log) + "B"),
                            ("--la_use_dcache=0"),
                            ("--la_use_l2cache=1"),
                            ("--la_cache_banks=" + str(banks)),
                            ("--la_cache_size=" + str(1 << la_size_log) + "B")
                        ],
                        params)

def scratch_sweep(args, params):
    for scratch_line in [128]: #[32, 64, 128]:
        for ssize_log in [16]: #[16,17,18]:
            params["ssize_log"] = str(ssize_log)
            params["scratch_line"] = str(scratch_line)

            la_cache_sweep(
                args + [
                    ("--scratch_size=" + str(1 << ssize_log) + "B"),
                    ("--scratch_line_size=" + str(scratch_line))],
                params)

            dcache_sweep(
                args + [
                    ("--scratch_size=" + str(1 << ssize_log) + "B"),
                    ("--scratch_line_size=" + str(scratch_line))],
                params)

def workload_sweep(args, params):
    for size in [32,64,128,256,512,1024]: #[32,64,128,256,512,1024]:
        for use_scratch in ['YES']: #['NO', 'YES']:
            for use_panel in ['YES']: #['NO', 'YES']:
                params["size"] = str(size)
                params["use_scratch"] = use_scratch
                params["use_panel"] = use_panel

                if(use_scratch == 'YES'):
                    scratch_sweep(args, params)
                else:
                    params["ssize_log"] = str(0)
                    params["scratch_line"] = str(0)
                    la_cache_sweep(args, params)
                    dcache_sweep(args, params)

###########################################################################
# START PARAMETER SWEEP
###########################################################################
args = []
params = {}
workload_sweep(args, params)

