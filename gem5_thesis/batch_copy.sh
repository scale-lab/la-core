
###########################################################################
# MUST DEFINE THESE EXTERNAL ENVIRONMENT VARIABLES:
#   BATCH_IPS: list of ips to run work on
#   BATCH_SSH_HOSTS: list of host names from ssh config file
#   BATCH_USER: the user to ssh as 
#   BATCH_FS_ROOT: where gem5 should be copied to
#   BATCH_JOBS_PER_NODE: how many parallel jobs can run on remote nodes
#
# ARGS TO PASS IN:
#   arg1: (required) the workload to run on gem5 while sweeping parameters
#   arg2: (optional) the sweep filter to pass to parameter_sweep.sh
###########################################################################

FS=$BATCH_FS_ROOT
JOBS=$BATCH_JOBS_PER_NODE
WKLOAD=$1
SWEEP_TYPE=$2
GEM5="build/RISCV_LA_CORE/gem5.opt"
PSWEEP="parameter_sweep.sh"

###########################################################################
# Find any available remote nodes
###########################################################################

SSH_HOSTS=""
IPS_ARRAY=($BATCH_IPS)
SSH_ARRAY=($BATCH_SSH_HOSTS)
count=${#IPS_ARRAY[@]}

for i in `seq 1 $count`; do
  ip=${IPS_ARRAY[$i-1]}
  ssh_host=${SSH_ARRAY[$i-1]}
  if ping -c 1 -W 1 $ip &> /dev/null;
  then
    echo "ip $ip found"
    SSH_HOSTS="${SSH_HOSTS}${BATCH_USER}@$ssh_host "
  else
    echo "ip $ip unreachable"
  fi
done
echo "SSH_HOSTS=$SSH_HOSTS"

###########################################################################
# Update the files on their machines
###########################################################################

COPY_FILES="SConstruct COPYING LICENSE README"
COPY_DIRS=".git configs build_opts src ext system util tests"
PCMD="parallel --no-notice -j 100"

#NOTE: deleting is not necessary anymore, since we recompile remotely
#$PCMD ssh {1} "rm -fr $FS" ::: $SSH_HOSTS

#$PCMD rm -fr "{1}.tar.gz" 2>/dev/null ::: $COPY_DIRS
$PCMD ssh {2} "mkdir -p $FS/{1}" ::: $COPY_DIRS ::: $SSH_HOSTS
$PCMD rsync -avz --exclude='*.pyc' {1} {2}:$FS/ ::: $COPY_DIRS ::: $SSH_HOSTS
$PCMD scp "$COPY_FILES" "{1}:$FS/" ::: $SSH_HOSTS
$PCMD scp "$PSWEEP" "{1}:$FS/" ::: $SSH_HOSTS
$PCMD scp "$WKLOAD" "{1}:$FS/" ::: $SSH_HOSTS
echo "initialized state for transfer"

#$PCMD tar czf "{1}.tar.gz" {1} ::: $COPY_DIRS
#echo "created tarballs"

#$PCMD scp "{1}.tar.gz {2}:$FS/{1}.tar.gz" ::: $COPY_DIRS ::: $SSH_HOSTS
#echo "scp transfer complete"

#$PCMD ssh {2} "tar xzf $FS/{1}.tar.gz -C $FS" ::: $COPY_DIRS ::: $SSH_HOSTS
#echo "unzipped remote files"

###########################################################################
# BUILD gem5
###########################################################################

set -x
#$PCMD ssh {1} "cd $FS; scons -j $JOBS $GEM5" ::: $SSH_HOSTS
#echo "built gem5.opt"

###########################################################################
# run the job in parallel
###########################################################################

#echo "starting parameter_sweep"
#$PCMD ssh {1} "cd $FS; ./$PSWEEP $JOBS $WKLOAD $SWEEP_TYPE" ::: $SSH_HOSTS


###########################################################################
# gather results from all the nodes
###########################################################################

