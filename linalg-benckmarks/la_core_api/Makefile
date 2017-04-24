##############################################################################
# Tests for the RISC-V LACore header-only library 
#
# Different tests for the LACore header-only library
#
# test_api: don't run this, just build it an objdump it and verify that the
#           dumped assembly is what you expect
#
# test_data_movement_dp: test suite to run on the RISC-V LACore target 
#                        machine (or simulator) to verify that all types of
#                        data movements of double-precision scalars and 
#                        vectors works correctly
#
# test_data_execution_dp: test suite to run on the RISC-V LACore target 
#                         machine (or simulator) to verify that all types of
#                        	data execution of double-precision scalars and 
#                         vectors works correctly
##############################################################################

ROOT=$(RISCV)/bin/riscv64-unknown-elf

SRCDIR=test
OUTDIR=out

CFLAGS=-O3 -Iinclude -I../common/include
LFLAGS=-O3 -Wa,-march=rv64imafdxlacore
LIBS=-lm

TESTS=test_api test_data_movement_dp test_data_execution_dp \
							 test_data_movement_sp test_data_execution_sp
 
all: $(TESTS)

$(TESTS): $(addsuffix, .c,$(TESTS))
	$(ROOT)-gcc $(CFLAGS) -S -o $(OUTDIR)/$@.S $(SRCDIR)/$@.c
	$(ROOT)-gcc $(LFLAGS) -o $(OUTDIR)/$@ $(OUTDIR)/$@.S $(LIBS)
  
clean:
	rm -fr $(OUTDIR)/*
	@touch $(OUTDIR)/.keep
