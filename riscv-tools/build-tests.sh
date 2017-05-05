#! /bin/bash
#
# Script to build RISC-V ISA simulator, proxy kernel, and GNU toolchain.
# Tools will be installed to $RISCV.

. build.common

echo "Starting RISC-V Tests build process"

build_project riscv-tests --prefix=$RISCV/riscv64-unknown-elf

echo -e "\\nRISC-V Tests installation completed!"
