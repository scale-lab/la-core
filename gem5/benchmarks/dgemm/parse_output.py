#!/usr/bin/env python

import glob
import re

output = ""

keys = ["size", "useSCH", "usePNL", "vec", "simd",
        "ssize", "sline",
        "line", "useL1", "useL2", "dsize", "l2size",
        "lasize", "labank",
        "ave GFLOPs", "peak GFLOPs",
        "gsl ave GFLOPs", "gsl peak GFLOPs"]

for key in keys:
    output += key + "\t"

output += "\n"

for fname in glob.glob("./output/size=*"):
    with open(fname, "r") as infile:
        count = summed = peak = 0
        gsl_count = gsl_summed = gsl_peak = 0

        for line in infile.read().split("\n"):
            if(line.startswith("N=")):
                matches = re.findall(r"([\d\.]+) MFLOP/s", line)
                #LACore
                if(len(matches)):
                    val = float(matches[0])
                    count += 1
                    summed += val
                    if(val > peak):
                        peak = val
                if(len(matches) == 2):
                    gsl_val = float(matches[1])
                    gsl_count += 1
                    gsl_summed += gsl_val
                    if(gsl_val > gsl_peak):
                        gsl_peak = gsl_val

        for pair in fname.split("_"):
            output += pair.split("=")[1] + "\t"

        #LACore
        if(count == 0):
            output += str(0.0) + "\t"
        else:
            output += str(summed / count / 1000.0) + "\t"
        output += str(peak / 1000.0) + "\t"

        #gsl
        if(gsl_count == 0):
            output += str(0.0) + "\t"
        else:
            output += str(gsl_summed / gsl_count / 1000.0) + "\t"
        output += str(gsl_peak / 1000.0) + "\t"
        output += "\n"

with open("parsed.tsv", "w") as outfile:
    outfile.write(output)
