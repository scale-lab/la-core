# LACore Master Repository

LACore source is organized into the following:

- __linalg-benchmarks__: contains the LACoreAPI framework, the HPCC benchmarks for LACore, x86, RISC-V and CUDA
- __gem5__: The gem5 repository snapshot with the LACore additions
- __riscv-tools__: The complete RISC-V toolchain modified for the LACore. Only a handful of the sub-repositories have been modified
- __sched-framework__: Task scheduling framework for multicore LACore architectures. This is work-in-progress at the moment.

The remainder of this file is an installation guide. For the software developer docs (if you want to write C-programs targeting the LACore), read the [LACoreAPI Developer Docs](LACoreAPI-Docs.md)

# Installing
This guide has been verified on ubuntu 14.04 only. Feel free to pioneer other platforms.

## Installing riscv-tools
First, you should read through and understand the installation guide for the [riscv-tools meta-repository](https://github.com/riscv/riscv-tools). You will __NOT__ be following it exactly, since the submodules have been merged into this repository.

First, put the following in your `.bashrc` file:

    export RISCV=/path/to/install/riscv/toolchain
    export PATH=$PATH:$RISCV/bin
    
Then, install the ubuntu packages from the [riscv-tools](https://github.com/riscv/riscv-tools) guide, which at the time of this writing were:

    sudo apt-get install autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev


Then, `cd` into the riscv-tools directory and run the following build scripts:

    ./build.sh
    
This will build the newlib cross-compiler, which is what you want. you __don't__ want to build the glibc cross-compiler. Also, `spike` and `pk` are built along with the compiler. You will need these for functional simulation later.

## Testing the riscv toolchain

To make sure the RISC-V compiler with the LACore extension is working correctly, we will first cross-compile the test scripts in the linalg-benchmarks repository, and then we will run them on the `spike` functional simulator

First, change directories into `linalg-benchmarks/la_core_api`. Then run the following command:

    make test_api \
         test_data_movement_dp \
         test_data_execution_dp \
         test_data_movement_sp \
         test_data_execution_sp

This will build all the functional test scripts to make sure the LACoreAPI can be compiled successfully. The purpose of `test_api` is to be inspected by objdump manually to make sure everything looks correct (this was done for initial ISA testing and probably isn't relevant anymore...). You can view the binary using the following command:

    riscv64-unknown-elf-objdump -D out/test_api | less
    
This will also give you a good idea of how the LACoreAPI gets compiled to the binary. Next, we want to test the other four LACoreAPI test files using the spike simulator. Run the following 4 commands in your shell, and make sure all tests are passing for each of them:

    SCRATCH_SIZE=16 spike --extension=la_core pk out/test_data_movement_dp
    SCRATCH_SIZE=16 spike --extension=la_core pk out/test_data_execution_dp
    SCRATCH_SIZE=16 spike --extension=la_core pk out/test_data_movement_sp
    SCRATCH_SIZE=16 spike --extension=la_core pk out/test_data_execution_sp
    
The `SCRATCH_SIZE=16` is a bad hack to pass parameters to the LACore extension within the spike simulator by using environment variables instead of command line arguments. It means the scratchpad to simulate should be 2^16 bytes, or 64 kB. You can therefore make the scratchpad size any power of 2.

## cross-compiling GSL for RISC-V

The next step is cross-compiling GNU Scientific Library for RISC-V. We need GSL in order to run the HPCC benchmarks on the RISC-V platform, since we use GSL to verify the correctness of the LACore results. First, download [GSL sources](https://www.gnu.org/software/gsl/). Then run the following:

    ./configure --host=riscv64-unknown-elf --prefix=$RISCV
    make
    make install

You should now have `libgsl.a` and `libgslcblas.a` in `$RISCV/lib`. Now we can link against them when we cross-compile for the RISC-V platform.

## Building the HPCC benchmarks

Now we will cross-compile the modified HPCC benchmarks for the LACore to be run on the spike simulator. We will worry about gem5 after we can get the functional simulation of the benchmarks to pass. First change directories into `linalg-benchmarks/benchmarks`. Then run the following:

    make dgemm_la_core_sweep \
     dstream_la_core_sweep \
     dfft_la_core_sweep \
     drandom_access_la_core_sweep \
     dlu_solve_la_core_sweep \
       ptrans_la_core_sweep \
     dtrsm_la_core_sweep 
       
This will build the 6 modified HPCC benchmarks (and DTRSM, a BLAS-3 routine) and put the output binaries in the `out` folder. Each of the 7 benchmarks takes slightly different parameters and you should just look at the `main()` function for each of them to figure out whats going on. Its not too complicated. For starters, here are simple command lines to run each of the benchmarks on the spike functional simulator using a 64 kB scratchpad and arbitrary workload sizes. __NOTE__: you can add a `--debug` flag to all the benchmarks for more verbose output if something seems wrong. __NOTE__: all matrix and vector data is randomly generated floating point numbers. The random number generator can use a different seed if you pass in `--seed=X`, where `X` is a positive integer.

The following will run DGEMM with 64x64 sized matrices, and a block size of 64x64 (you can change the block size if you want).

    SCRATCH_SIZE=16 spike --extension=la_core pk out/dgemm_la_core_sweep --size=64 --bs=64 --scratch_size=16
    
The following will run the STREAM benchmark with vector sizes of 2^12
    
    SCRATCH_SIZE=16 spike --extension=la_core pk out/dstream_la_core_sweep --size=12 --scratch_size=16
    
The following will run the 1-D FFT benchmark with a vector size of 2^12
    
    SCRATCH_SIZE=16 spike --extension=la_core pk out/dfft_la_core_sweep --log_size=12 --scratch_size=16
    
The following will run the Random Access benchmark with a table size of 2^16. Note that the application doesn't require `--scratch_size` but spike still requires `SCRATCH_SIZE=16`.
    
    SCRATCH_SIZE=16 spike --extension=la_core pk out/drandom_access_la_core_sweep --log_size=16
    
The following will run the HPL benchmark with a matrix size of 64x64
    
    SCRATCH_SIZE=16 spike --extension=la_core pk out/dlu_solve_la_core_sweep --log_size=6 --scratch_size=16
    
The following will run the PTRANS benchmark with a matrix size of 64x64
    
    SCRATCH_SIZE=16 spike --extension=la_core pk out/ptrans_la_core_sweep --log_size=6 --scratch_size=16
    
The following will run the DTRSM kernel with a matrix size of 64x64

    SCRATCH_SIZE=16 spike --extension=la_core pk out/dtrsm_la_core_sweep --size=64 --scratch_size=16
    
## Building LACore gem5 models

If everything is working up to this point, we are ready to build gem5 and then run the HPCC benchmarks on the gem5 to get cycle-accurate results. First, you are required to read through [this tutorial](http://learning.gem5.org/book/part1/building.html) on setting up system for running gem5. Don't actually build anything, just make sure the packages are installed and environment variables are set correctly. Then, change directories into `gem5`. Then, run the following command:

    scons -j10 build/RISCV_LA_CORE/gem5.opt
    
And wait for a long time, Replace `10` here with the number of cores on your machine. If this finishes successfully, you now can run the HPCC benchmarks using any of the following CPU models:

- __AtomicLACoreSimpleCPU__: everything instruction takes 1 cycle
- __TimingLACoreSimpleCPU__: timing accurate, but only 1-deep pipeline
- __MinorLACoreCPU__: pipelined, timing accurate LACore CPU model

You'll most likely want to use the `MinorLACoreCPU` model. In order to run these models, use the configuration scripts found in the `gem5/configs/la_core` folder. The following examples will use the `MinorLACoreCPU` model, but use the other python scripts to run the `AtomicLACoreSimpleCPU` and `TimingLACoreSimpleCPU` CPU models. The config scripts are respectively:

- configs/la_core/atomic_simple_la_core.py
- configs/la_core/full_timing_la_core.py
- configs/la_core/minor_la_core.py

To run the HPCC benchmarks and DTRSM on the pipelined LACore model in gem5, with the same workload arguments as above, use the following command lines:

    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/dgemm_la_core_sweep --size=64 --bs=64 --scratch_size=16"
    
    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/dstream_la_core_sweep --size=12 --scratch_size=16"
    
    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/dfft_la_core_sweep --log_size=12 --scratch_size=16"
    
    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/drandom_access_la_core_sweep --log_size=16"
    
    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/dlu_solve_la_core_sweep --log_size=6 --scratch_size=16"
    
    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/ptrans_la_core_sweep --log_size=6 --scratch_size=16"
    
    /build/RISCV_LA_CORE/gem5.opt ./configs/la_core/minor_la_core.py --cmd="../linalg-benchmarks/benchmarks/out/dtrsm_la_core_sweep --size=64 --scratch_size=16"
    
If everything is passing, you have successfully installed the full LACore development environment, congratulations. You can now start writing your own benchmarks and programs and running them on spike and gem5 using the same workflow described here.

# Building x86 HPCC benchmarks

The HPCC benchmarks have also been written for the x86 superscalar platform. to build these, you need to install GSL and FFTW3 using `apt-get` or your package manager. Then, change directories into `linalg-benchmarks/benchmarks` and run the following command:

    make dgemm_x86_sweep \
         dstream_x86_sweep \
         dfft_x86_sweep \
         drandom_access_x86_sweep \
         dlu_solve_x86_sweep \
         ptrans_x86_sweep

To verify they work, you can run them directly on your real machine such as:

    ./out/dgemm_x86_sweep --size=64 --bs=64
    ./out/dstream_x86_sweep --size=4096
    ./out/dfft_x86_sweep --log_size=12
    ./out/drandom_access_x86_sweep --log_size=12
    ./out/dlu_solve_x86_sweep --log_size=6
    ./out/ptrans_x86_sweep --log_size=6
    
# Building x86 Superscalar gem5 model

Now we want to build the gem5 x86 superscalar model and run the x86 HPCC benchmarks using that configuration, since it gives us greater control over the x86 system parameters. You should already have gem5 setup and configured if you followed the previous gem5 section. Now, change directories into `gem5` and run the following command:

    scons -j10 build/X86/gem5.opt
    
This will build the x86 models for you. Again, change `j10` to something that makes sense for your machine. After this finishes, you can run the HPCC benchmarks using the X86 configuration file located in `gem5/configs/la_core/x86_O3`. For example, here are the gem5 command lines for the above x86 HPCC benchmarks that were run on your real machine:

    ./build/X86/gem5.opt ./configs/la_core/x86_O3 --cmd="../linalg-benchmarks/benchmarks/out/dgemm_x86_sweep --size=64 --bs=64
    
    ./build/X86/gem5.opt ./configs/la_core/x86_O3 --cmd="../linalg-benchmarks/benchmarks/out/dstream_x86_sweep --size=4096
   
    ./build/X86/gem5.opt ./configs/la_core/x86_O3 --cmd="../linalg-benchmarks/benchmarks/out/dfft_x86_sweep --log_size=12
    
    ./build/X86/gem5.opt ./configs/la_core/x86_O3 --cmd="../linalg-benchmarks/benchmarks/out/drandom_access_x86_sweep --log_size=12
    
    ./build/X86/gem5.opt ./configs/la_core/x86_O3 --cmd="../linalg-benchmarks/benchmarks/out/dlu_solve_x86_sweep --log_size=6
    
    ./build/X86/gem5.opt ./configs/la_core/x86_O3 --cmd="../linalg-benchmarks/benchmarks/out/ptrans_x86_sweep --log_size=6
    
If all of these work, you have successfully built and run the x86 benchmarks in gem5.

## Building CUDA HPCC Benchmarks

This was a painful setup and debugging process, and others should beware that running the CUDA HPCC in gem5-gpu may not be worth the time you will spend setting this up, debugging and waiting for the very slow gpgpu-sim simulation results.

For the undeterred, first read the [gem5-gpu getting started wiki](https://gem5-gpu.cs.wisc.edu/wiki/start) to understand the general gist of how gem5 and gpgpu-sim are working together here. As you see, you need CUDA Toolkit 3.2 and gcc 4.7 to be installed. I used [Vagrant](https://www.vagrantup.com/) to setup an isolated environment since so many build tools and environment variables are getting messed around with here. I would recommend the same.

After you get CUDA 3.2 and gcc 4.7 installed correctly in your Vagrant Box, you should __COPY__ the linalg-benchmarks directory into the `gem5-gpu/benchmarks` folder, since we need to link against a bunch of libraries in there. This is a terrible workflow and needs to be fixed. 

Also, you need to install gem5-gpu inside the Vagrant Box by following the [installation guide wiki](https://gem5-gpu.cs.wisc.edu/wiki/start). Ok, now you can try to build the CUDA binaries inside the Vagrant Box using the following commands:

    make dgemm_cuda_sweep \
         dstream_cuda_sweep \
         dfft_cuda_sweep \
         drandom_access_cuda_sweep \
         dlu_solve_cuda_sweep \
         ptrans_cuda_sweep

If this works, you now have binaries you can run on gem5-gpu. If not, you have some debugging to do. To run the binaries on the gem5-gpu simulator, change directories into `gem5` (within Vagrant Box!), and run something similar to the following command lines:

    ./build/X86_VI_hammer_GPU/gem5.opt ../gem5-gpu/configs/se_fusion.py --clusters=1 --cores_per_cluster=2 --gpu-core-clock=1GHz --cpu-type=detailed --mem-size=8GB --cpu-clock=3GHz --caches --l2cache --access-host-pagetable -c /home/vagrant/gem5-gpu/benchmarks/linalg-benchmarks/benchmarks/out/ptrans_cuda_sweep -o "--log_size=6"
    
You MUST use absolute paths to your workload (bug in gem5-gpu), and you MUST pass in your arguments to the workload using the `-o` flag. Command lines for DGEMM, FFT, STREAM, Random Access and HPL are all the same as for PTRANS above, except the `-o` workload arguments differ. refer to the source files for exact usage.

# Developing
## Developing gem5

The LACore extension touches a lot of c++ source files in the gem5 respository, but mainly, you should be looking at `src/arch/RISC-V` for ISA changes and `src/cpu/la_core` for micro-architectural changes. Sometimes, you will need to change both of them if new instructions with new functionality are added to the ISA.

After you make changes, you just build using `scons -j10 build/RISCV_LA_CORE/gem5.gpu` as described in the installation section

To modify the configuration scripts, which are written in python. These scripts just hook together the hardware pieces right before simulation, and are useful for parameter sweeps. You will only need to edit these if you add a new parameter to any of the SimObject Python files in the src/ directory. 

## Modifying gcc

In order to extend the ISA, you need to modify the `riscv-opcodes` sub-directory first, and then run `make` inside it to update all the `encoding.h` files in the rest of the riscv-tools sub-directories. Then, go into `riscv-gnu-toolchain/riscv-binutils-gdb/gas/config/tc-riscv.c` and make most of your changes there. You might also need to look at the following files:

    riscv-gnu-toolchain/riscv-binutils-gdb/gas/config/tc-riscv.h
    riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv.h
    riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.h
    riscv-gnu-toolchain/riscv-binutils-gdb/opcodes/riscv-dis.c
    riscv-gnu-toolchain/riscv-binutils-gdb/opcodes/riscv-opc.c
    
After you make the changes you want, change directories up to the `riscv-tools` repository and run

    ./build-binutils-gdb
    
This will rebuild the gcc toolchain (with your additions to gas), and update the binaries in `$RISCV/bin`. Now, you are __STRONGLY__ advised to rebuild and rerun the LACoreAPI regression tests in spike to make sure the assembler isn't broken!
