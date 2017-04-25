#
# Copyright (c) 2017 Samuel Steffl
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Samuel Steffl
#

CC = gcc -O3

CFLAGS_VERBOSE = -D_REENTRANT $(DEBUGFLAGS) -D_GNU_SOURCE -D_XOPEN_SOURCE=500 \
    -std=c99 -Iinclude -Itest/include -DCOLOR -D__BSD_VISIBLE -DDBG_MODES=all

CFLAGS_RELEASE = -D_REENTRANT $(DEBUGFLAGS) -D_GNU_SOURCE -D_XOPEN_SOURCE=500 \
    -std=c99 -Iinclude -Itest/include -DCOLOR -D__BSD_VISIBLE -DDBG_MODES=-all

OUT_DIR = bin
SRC_DIR = src
TEST_DIR = test

define contained
$(addsuffix .o,$(basename $(wildcard $(1)/*.[cS])))
endef

define indirs
$(foreach dr,$(1),$(call contained,$(dr)))
endef

define srcdirs
$(call indirs,$(foreach dr,$(1),$(SRC_DIR)/$(dr)))
endef

#default
CFLGAS = $(CFLAGS_RELEASE)

ifeq ($(MAKECMDGOALS),debug)
	CFLAGS = $(CFLAGS_IP)
endif
ifeq ($(MAKECMDGOALS),release)
	CFLAGS = $(CFLAGS_IP_VERBOSE)
endif

MAIN_CC = $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $(OUT_DIR)/$@

################################ COMMANDS ##############################
all: test_synch_1_group_1_cpu_1_task
	
test_synch_1_group_1_cpu_1_task: $(call srcdirs,util sched) \
	                               $(TEST_DIR)/test_synch_1_group_1_cpu_1_task.o
	$(MAIN_CC)

#_______________________________ OBJECT FILES ______________________________#
%.o: %.c .FORCE
	$(CC) -c $(CFLAGS) $< -o $@
	
.FORCE:

clean:
	@ echo "Deleting:"
	@ find . -depth -name "*.o" -print -delete
	@ rm -rf $(OUT_DIR)/*
	@ touch $(OUT_DIR)/.keep

