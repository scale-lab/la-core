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
RISCVCFLAGS=-O3 -DRISCV=1 -Iinclude -I../la_core_api/include \
            -I../common/include -I$(RISCV)/include
RISCVLFLAGS=-Wa,-march=rv64imafdxlacore -L$(RISCV)/lib
RISCVLIBS=-lgsl -lgslcblas -lm

#flags for compiling x86 binaries
CCx86=/usr/bin/gcc
CPPx86=/usr/bin/g++
x86CFLAGS=-O3 -DRISCV=1 \
          -Iinclude -I../la_core_api/include -I../common/include -std=c99 \
          -I../eigen3.3.3 \
          -mfpmath=both \
          -msse2 -ffast-math -ftree-vectorize -ftree-vectorizer-verbose=6
x86LIBS=-lgsl -lgslcblas -static -lfftw3 -lm
#x86LIBS=-lgsl -lcblas -lopenblas -lm

#flags for compiling CUDA binaries (nvcc 3.2, for gem5-gpu platform, gcc 4.4)
LIBDIR   =$(NVIDIA_CUDA_SDK_LOCATION)/lib
COMMONDIR=$(NVIDIA_CUDA_SDK_LOCATION)/common
SDKINCDIR=$(NVIDIA_CUDA_SDK_LOCATION)/common/inc/
NVCC=nvcc
CUDACXX=g++
CUDACFLAGS=-O3 -c -arch sm_20 --keep --compiler-options -fno-strict-aliasing \
           -DGEM5_FUSION -I. -I$(CUDAHOME)/include/ -I$(SDKINCDIR) \
           -Iinclude -I../common/include \
           -I../../../gem5/util/gem5 -I../../libcuda -DUNIX -DRISCV=1
CUDACXXFLAGS=-static -L$(LIBDIR) -L$(CUDAHOME)/lib -L../../libcuda \
						 -lcutil_x86_64 -lcuda -lm5op_x86 -DUNIX

all: dgemm_la_core_sweep 					\
		 dtrsm_la_core_sweep 					\
		 dstream_la_core_sweep 				\
		 dfft_la_core_sweep 					\
		 drandom_access_la_core_sweep \
		 dlu_solve_la_core_sweep 			\
		 															\
		 dgemm_x86_sweep 							\
		 dstream_x86_sweep 						\
		 dfft_x86_sweep 							\
		 drandom_access_x86_sweep 		\
		 dlu_solve_x86_sweep 					\
		 															\
		 #sgemm_la_core_sweep 			  \	


cuda: dgemm_cuda_sweep 						\
		  dstream_cuda_sweep 					\
		  dfft_cuda_sweep 						\
		  drandom_access_cuda_sweep 	\
		  dlu_solve_cuda_sweep 				\


##############################################################################
# HPCC LACore Double-precision binaries 
##############################################################################

dgemm_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/gemm/dgemm_la_core.c \
    $(SRCDIR)/dgemm_la_core_sweep.c \
    $(SRCDIR)/transpose/dtranspose_la_core.c \
    -o $(OUTDIR)/dgemm_la_core_sweep \
    $(RISCVLIBS)

dtrsm_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/trsm/dtrsm_la_core.c \
    $(SRCDIR)/dtrsm_la_core_sweep.c \
    -o $(OUTDIR)/dtrsm_la_core_sweep \
    $(RISCVLIBS)

dstream_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/stream/dstream_la_core.c \
    $(SRCDIR)/dstream_la_core_sweep.c \
    -o $(OUTDIR)/dstream_la_core_sweep \
    $(RISCVLIBS)

dfft_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/fft/dfft_la_core.c \
    $(SRCDIR)/dfft_la_core_sweep.c \
    -o $(OUTDIR)/dfft_la_core_sweep \
    $(RISCVLIBS)

drandom_access_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/random_access/drandom_access_la_core.c \
    $(SRCDIR)/drandom_access_la_core_sweep.c \
    -o $(OUTDIR)/drandom_access_la_core_sweep \
    $(RISCVLIBS)

dlu_solve_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/lu_solve/dlu_solve_la_core.c \
    $(SRCDIR)/trsm/dtrsm_la_core.c \
    $(SRCDIR)/dlu_solve_la_core_sweep.c \
    -o $(OUTDIR)/dlu_solve_la_core_sweep \
    $(RISCVLIBS)

ptrans_la_core_sweep:
	$(RISCVCC) $(RISCVCFLAGS) $(RISCVLFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/ptrans_la_core_sweep.c \
    -o $(OUTDIR)/ptrans_la_core_sweep \
    $(RISCVLIBS)

##############################################################################
# HPCC x86 Double-precision binaries 
##############################################################################

dgemm_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/dgemm_x86_sweep.c \
    -o $(OUTDIR)/dgemm_x86_sweep \
    $(x86LIBS)

dstream_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/dstream_x86_sweep.c \
    -o $(OUTDIR)/dstream_x86_sweep \
    $(x86LIBS)

dfft_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/dfft_x86_sweep.c \
    -o $(OUTDIR)/dfft_x86_sweep \
    $(x86LIBS)

drandom_access_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/drandom_access_x86_sweep.c \
    -o $(OUTDIR)/drandom_access_x86_sweep \
    $(x86LIBS)

dlu_solve_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/dlu_solve_x86_sweep.c \
    -o $(OUTDIR)/dlu_solve_x86_sweep \
    $(x86LIBS)

ptrans_x86_sweep:
	$(CPPx86) $(x86CFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/ptrans_x86_sweep.c \
    -o $(OUTDIR)/ptrans_x86_sweep \
    $(x86LIBS)

##############################################################################
# HPCC CUDA Double-precision binaries 
##############################################################################

dgemm_cuda_sweep:
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/matrix_utilities.c \
    -o $(OBJDIR)/matrix_utilities.o
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/dgemm_cuda_sweep.cu \
    -o $(OBJDIR)/dgemm_cuda_sweep.o
	python ../../common/sizeHack.py -f dgemm_cuda_sweep.cu.cpp -t sm_20
	$(CUDACXX) \
    -o $(OUTDIR)/dgemm_cuda_sweep \
       ./dgemm_cuda_sweep.cu.cpp \
       $(OBJDIR)/matrix_utilities.o \
			 $(CUDACXXFLAGS)

dstream_cuda_sweep:
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/matrix_utilities.c \
    -o $(OBJDIR)/matrix_utilities.o
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/dstream_cuda_sweep.cu \
    -o $(OBJDIR)/dstream_cuda_sweep.o
	python ../../common/sizeHack.py -f dstream_cuda_sweep.cu.cpp -t sm_20
	$(CUDACXX) \
    -o $(OUTDIR)/dstream_cuda_sweep \
       ./dstream_cuda_sweep.cu.cpp \
       $(OBJDIR)/matrix_utilities.o \
			 $(CUDACXXFLAGS)

dfft_cuda_sweep:
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/matrix_utilities.c \
    -o $(OBJDIR)/matrix_utilities.o
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/dfft_cuda_sweep.cu \
    -o $(OBJDIR)/dfft_cuda_sweep.o
	python ../../common/sizeHack.py -f dfft_cuda_sweep.cu.cpp -t sm_20
	$(CUDACXX) \
    -o $(OUTDIR)/dfft_cuda_sweep \
       ./dfft_cuda_sweep.cu.cpp \
       $(OBJDIR)/matrix_utilities.o \
			 $(CUDACXXFLAGS)

drandom_access_cuda_sweep:
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/matrix_utilities.c \
    -o $(OBJDIR)/matrix_utilities.o
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/drandom_access_cuda_sweep.cu \
    -o $(OBJDIR)/drandom_access_cuda_sweep.o
	python ../../common/sizeHack.py -f drandom_access_cuda_sweep.cu.cpp -t sm_20
	$(CUDACXX) \
    -o $(OUTDIR)/drandom_access_cuda_sweep \
       ./drandom_access_cuda_sweep.cu.cpp \
       $(OBJDIR)/matrix_utilities.o \
			 $(CUDACXXFLAGS)

dlu_solve_cuda_sweep:
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/matrix_utilities.c \
    -o $(OBJDIR)/matrix_utilities.o
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/dlu_solve_cuda_sweep.cu \
    -o $(OBJDIR)/dlu_solve_cuda_sweep.o
	python ../../common/sizeHack.py -f dlu_solve_cuda_sweep.cu.cpp -t sm_20
	$(CUDACXX) \
    -o $(OUTDIR)/dlu_solve_cuda_sweep \
       ./dlu_solve_cuda_sweep.cu.cpp \
       $(OBJDIR)/matrix_utilities.o \
			 $(CUDACXXFLAGS)

ptrans_cuda_sweep:
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/matrix_utilities.c \
    -o $(OBJDIR)/matrix_utilities.o
	$(NVCC) $(CUDACFLAGS) \
       $(SRCDIR)/ptrans_cuda_sweep.cu \
    -o $(OBJDIR)/ptrans_cuda_sweep.o
	python ../../common/sizeHack.py -f ptrans_cuda_sweep.cu.cpp -t sm_20
	$(CUDACXX) \
    -o $(OUTDIR)/ptrans_cuda_sweep \
       ./ptrans_cuda_sweep.cu.cpp \
       $(OBJDIR)/matrix_utilities.o \
			 $(CUDACXXFLAGS)

##############################################################################
# other binaries
##############################################################################

sgemm_la_core_sweep:
	$(CCRISCV) -DRISCV=1 $(CFLAGS) $(LFLAGS) \
    $(SRCDIR)/matrix_utilities.c \
    $(SRCDIR)/gemm/sgemm_la_core.c \
    $(SRCDIR)/sgemm_la_core_sweep.c \
    $(SRCDIR)/transpose/stranspose_la_core.c \
    -o $(OUTDIR)/sgemm_la_core_sweep \
    $(LIBS)

##############################################################################
# Clean
##############################################################################

clean:
	rm -fr $(OBJDIR)/*
	@touch $(OBJDIR)/.keep
	rm -fr $(OUTDIR)/*
	@touch $(OUTDIR)/.keep
