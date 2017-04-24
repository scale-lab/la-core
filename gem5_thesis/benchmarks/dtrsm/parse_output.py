#!/usr/bin/env python

import glob
import re

output = ""

keys = ["size", "useSCH", "vec", "simd",
        "ssize", "sline",
        "line", "useL1", "useL2", "dsize", "l2size",
        "lasize", "labank",
        "ave norm GFLOPs", "ave trns GFLOPS", "peak GFLOPs",
        "gsl ave norm GFLOPs", "gsl ave trns GFLOPS", "gsl peak GFLOPs"]

for key in keys:
    output += key + "\t"

output += "\n"

for fname in glob.glob("./output/size=*"):
    with open(fname, "r") as infile:
        norm_count = norm_summed = trns_count = trns_summed = peak = 0
        gsl_norm_count = gsl_norm_summed = 0
        gsl_trns_count = gsl_trns_summed = gsl_peak = 0

        for line in infile.read().split("\n"):
            if(line.startswith("M=")):
                matches = re.findall(r"([\d\.]+) MFLOP/s", line)
                #LACore
                if(len(matches)):
                    val = float(matches[0])
                    if(('TU' in line) or ('TD' in line)):
                        trns_count += 1
                        trns_summed += val
                    else:
                        norm_count += 1
                        norm_summed += val
                    if(val > peak):
                        peak = val
                #gsl
                if(len(matches) == 2):
                    gsl_val = float(matches[1])
                    if(('TU' in line) or ('TD' in line)):
                        gsl_trns_count += 1
                        gsl_trns_summed += gsl_val
                    else:
                        gsl_norm_count += 1
                        gsl_norm_summed += gsl_val
                    if(gsl_val > gsl_peak):
                        gsl_peak = gsl_val

        for pair in fname.split("_"):
            output += pair.split("=")[1] + "\t"

        #LACore
        if(norm_count == 0):
            output += str(0.0) + "\t"
        else:
            output += str(norm_summed / norm_count / 1000.0) + "\t"
        if(trns_count == 0):
            output += str(0.0) + "\t"
        else:
            output += str(trns_summed / trns_count / 1000.0) + "\t"
        output += str(peak / 1000) + "\t"

        #gsl
        if(gsl_norm_count == 0):
            output += str(0.0) + "\t"
        else:
            output += str(gsl_norm_summed / gsl_norm_count / 1000.0) + "\t"
        if(gsl_trns_count == 0):
            output += str(0.0) + "\t"
        else:
            output += str(gsl_trns_summed / gsl_trns_count / 1000.0) + "\t"
        output += str(gsl_peak / 1000) + "\t"
        output += "\n"

with open("parsed.tsv", "w") as outfile:
    outfile.write(output)
