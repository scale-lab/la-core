#!/bin/bash

#beforehand, set up the read-only ssh deploy keys in the github repos
#  and add the keys to the ~/.ssh folder. name the remote samgithub

cd ~/scratch
mkdir -p SSTEFFL/gem5_thesis

cd SSTEFFL/gem5_thesis
git init
git remote add ssteffl git@samgithub:ssteffl/gem5_thesis.git
git fetch ssteffl
git checkout -b riscv_extension ssteffl/riscv_extension

#dont load protobuf/3.1.0, its broken
module load gcc/4.9.2
module load swig/3.0.8
module load python/2.7.13
module load scons/2.3.0
module load gsl/2.3
module load fftw/3.3.4
scons -j 10 build/RISCV_LA_CORE/gem5.opt
