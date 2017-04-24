#!/usr/bin/env bash

echo $@
module load gcc/4.9.2
module load fftw/3.3.4
module load gsl/2.3
"$@"
