###########################################################################
# Runs full or partial parameter sweeps for LACore full_timing in parallel
# Currently runs sweeps pertaining to:
#   - cache-line size, ld1_size, l2_size, la_cache_size, la_cache_banks
#   - with/without la_cache, and with/without l2cache under lacache
#   - with/without scratchpad, and scratchpad sizes
#   - datapath height
#
# Author: Samuel Steffl
###########################################################################
CMD="./build/RISCV_LA_CORE/gem5.opt ./configs/thesis/full_timing_la_core.py"
JOBS=$1
PROG=$2
TYPE=$3
MEM_SIZE="2048MB"

OUT="output_files"

#set -x
cd "$(dirname ${BASH_SOURCE[0]})"
echo "called with JOBS=$JOBS PROG=$PROG TYPE=$TYPE"

###########################################################################

echo "--------------------------------------------------------------------"
echo "testing with lacache no l2"
echo "--------------------------------------------------------------------"
parallel -j $JOBS \
         $CMD "'--cmd=$PROG --size={1} --bs={1}'" \
           --la_use_dcache=0 \
           --la_use_l2cache=0 \
           --cache_line_size={2} \
           --l2_size={3} \
           --la_cache_size={4} \
           --la_cache_banks={5} \
           --mem_size=$MEM_SIZE \
        ::: 32 64 128 256 512 \
        ::: 32 64 128 \
        ::: "128kB" "256kB" "512kB" \
        ::: "128kB" "256kB" "512kB" \
        ::: 1 2 4 8 16 \
        2>&1 \
  | grep -e "^N=" -e "^command line" \
  | perl -pe "s|^.+?([\d\.]+) MFLOP\/s.*\|\| gsl.+$|\1|g;" \
         -pe "s|^command line.*--cmd=(.*)$|\1|g"

###########################################################################

echo "--------------------------------------------------------------------"
echo "testing with lacache and l2"
echo "--------------------------------------------------------------------"
parallel -j $JOBS \
         $CMD "'--cmd=$PROG --size={1} --bs={1}'" \
           --la_use_dcache=0 \
           --la_use_l2cache=1 \
           --cache_line_size={2} \
           --l2_size={3} \
           --la_cache_size={4} \
           --la_cache_banks={5} \
           --mem_size=$MEM_SIZE \
        ::: 32 64 128 256 512 \
        ::: 32 64 128 \
        ::: "128kB" "256kB" "512kB" \
        ::: "64kB" "128kB" \
        ::: 1 2 4 8 16 \
        2>&1 \
  | grep -e "^N=" -e "^command line" \
  | perl -pe "s|^.+?([\d\.]+) MFLOP\/s.*\|\| gsl.+$|\1|g;" \
         -pe "s|^command line.*--cmd=(.*)$|\1|g"



###########################################################################

echo "--------------------------------------------------------------------"
echo "testing without la_cache"
echo "--------------------------------------------------------------------"
parallel -j $JOBS \
         $CMD "'--cmd=$PROG --size={1} --bs={1}'" \
           --la_use_dcache=1 \
           --la_use_l2cache=0 \
           --cache_line_size={2} \
           --l1d_size={3} \
           --l2_size={4} \
           --mem_size=$MEM_SIZE \
        ::: 32 64 128 256 512 \
        ::: 32 64 128 \
        ::: "32kB" "64kB" \
        ::: "256kB" "512kB" \
        2>&1 \
  | grep -e "^N=" -e "^command line" \
  | perl -pe "s|^.+?([\d\.]+) MFLOP\/s.*\|\| gsl.+$|\1|g;" \
         -pe "s|^command line.*--cmd=(.*)$|\1|g"


