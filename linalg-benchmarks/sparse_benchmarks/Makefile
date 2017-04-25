##############################################################################
# Tests for the RISC-V LACore header-only library 
#
# Different tests for the LACore header-only library
#
# dgemm_la_core_sweep: compiles the RISC-V LACore binary for dgemm. This 
#                      program sweeps all the relevant dgemm parameters and
#                      compares the results of the LACore to a known 
#                      correct implementation (gsl)
#
# NOTE: compiling gsl for riscv64-unknown-elf:
#  1) download https://www.gnu.org/software/gsl/
#  2) ./configure --host=riscv64-unknown-elf --prefix=$RISCV
#  3) make
#  3) make install
#  ### you should now have libgsl.a and libgslcblas.a in $RISCV/lib
#
##############################################################################

$(info uname=$(shell uname))


SRCDIR=src
OUTDIR=out
OBJDIR=objdir

#flags for compiling RISC-V binaries
RISCVCC=$(RISCV)/bin/riscv64-unknown-elf-gcc
RISCVCFLAGS=-O3 -DRISCV=1 -Iinclude -I../benchmarks/include \
            -I../la_core_api/include \
            -I../common/include -I$(RISCV)/include
RISCVLFLAGS=-Wa,-march=rv64imafdxlacore -L$(RISCV)/lib
RISCVLIBS=-lgsl -lgslcblas -lm

#flags for compiling x86 binaries
CCx86=/usr/bin/gcc
CPPx86=/usr/bin/g++
x86CFLAGS=-O3 -DRISCV=1 \
          -Iinclude -I../la_core_api/include -I../common/include -std=c99 \
          -I../benchmarks/include -I../eigen3.3.3 \
          -mfpmath=both \
          -msse2 -ffast-math -ftree-vectorize -ftree-vectorizer-verbose=6
x86LIBS=-lgsl -lgslcblas -static -lm
#x86LIBS=-lgsl -lcblas -lopenblas -lm

all: dgemv_spv_la_core_sweep


##############################################################################
# LACore Sparse Double-precision binaries 
##############################################################################

dgemv_spv_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/../../benchmarks/src/matrix_utilities.c \
    $(SRCDIR)/gemv_spv/dgemv_spv_la_core.c \
    $(SRCDIR)/dgemv_spv_la_core_sweep.c \
    -o $(OUTDIR)/dgemv_spv_la_core_sweep \
    $(RISCVLIBS)

##############################################################################
# X86 Sparse Double-precision binaries 
##############################################################################

dgemv_spv_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/../../benchmarks/src/matrix_utilities.c \
    $(SRCDIR)/dgemv_spv_x86_sweep.c \
    -o $(OUTDIR)/dgemv_spv_x86_sweep \
    $(x86LIBS)

##############################################################################
# Clean
##############################################################################

clean:
	rm -fr $(OBJDIR)/*
	@touch $(OBJDIR)/.keep
	rm -fr $(OUTDIR)/*
	@touch $(OUTDIR)/.keep
