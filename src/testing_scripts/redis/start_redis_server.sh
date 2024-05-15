#!/bin/bash

source global_dirs.sh
redis_dir=third_party/tmp/redis-6.2.13

rm *.rdb

if [ -z $NTHREADS ]; then
    NTHREADS=$(grep -c processor /proc/cpuinfo)
fi
export NTHREADS
NCPU_NODES=$(cat /sys/devices/system/node/has_cpu | awk -F '-' '{print $NF+1}')
NMEM_NODES=$(cat /sys/devices/system/node/has_memory | awk -F '-' '{print $NF+1}')
MEM_NODES=($(ls /sys/devices/system/node | grep node | awk -F 'node' '{print $NF}'))

CGROUP_NAME="htmm"
###### update DIR!
source global_dirs.sh

CONFIG_CXL_MODE=${MEMTIS_CXL_OPTION}


function func_cache_flush() {
    echo 3 | sudo tee /proc/sys/vm/drop_caches
    free
    return
}

function func_memtis_setting() {
    # memtis settings 
    echo 199 | tee /sys/kernel/mm/htmm/htmm_sample_period
    echo 100007 | tee /sys/kernel/mm/htmm/htmm_inst_sample_period
    echo 1 | tee /sys/kernel/mm/htmm/htmm_thres_hot
    echo 2 | tee /sys/kernel/mm/htmm/htmm_split_period
    echo 100000 | tee /sys/kernel/mm/htmm/htmm_adaptation_period
	 
    echo 2000000 | tee /sys/kernel/mm/htmm/htmm_cooling_period
	 
    echo 2 | tee /sys/kernel/mm/htmm/htmm_mode
    echo 500 | tee /sys/kernel/mm/htmm/htmm_demotion_period_in_ms
    echo 500 | tee /sys/kernel/mm/htmm/htmm_promotion_period_in_ms
    echo 4 | tee /sys/kernel/mm/htmm/htmm_gamma
    ###  cpu cap (per mille) for ksampled
    echo 30 | tee /sys/kernel/mm/htmm/ksampled_soft_cpu_quota


    if [[ "x${CONFIG_CXL_MODE}" == "xon" ]]; then
	${memtis_userspace}/scripts/set_uncore_freq.sh on
	echo "enabled" | tee /sys/kernel/mm/htmm/htmm_cxl_mode
    else
	${memtis_userspace}/scripts/set_uncore_freq.sh off
	echo "disabled" | tee /sys/kernel/mm/htmm/htmm_cxl_mode
    fi

    sudo echo ${thp_setting} | tee /sys/kernel/mm/transparent_hugepage/enabled
    sudo echo ${thp_setting} | tee /sys/kernel/mm/transparent_hugepage/defrag


}

function func_prepare() {
    echo "Preparing benchmark start..."
    # setting for cpu 
    
    sh -c "echo off > /sys/devices/system/cpu/smt/control"

    sleep 1

    echo "performance" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor


	sudo sysctl kernel.perf_event_max_sample_rate=100000

	# disable automatic numa balancing
	sudo echo 0 > /proc/sys/kernel/numa_balancing
	# set configs
	func_memtis_setting
	 
}

function func_main() {
    ${memtis_userspace}/bin/kill_ksampled  
    TIME="/usr/bin/time"
    rm /tmp/liblinear_initialized
    rm /tmp/liblinear_thrashed
     
    

    # make directory for/run-memtis results-liblinear
    mkdir -p ${results_DIR}/results-liblinear
    LOG_DIR=${results_DIR}/results-liblinear 

 

    # set memcg for htmm
    sudo ${memtis_userspace}/scripts/set_htmm_memcg.sh htmm remove
    sudo ${memtis_userspace}/scripts/set_htmm_memcg.sh htmm $$ enable
	 
	echo dram size is ${BENCH_DRAM} !!!!!!!!!!!!!!
    sudo ${memtis_userspace}/scripts/set_mem_size.sh htmm 0 ${BENCH_DRAM}
    sleep 2


     

     
    # flush cache
    func_cache_flush
    sleep 2

	 
	${memtis_userspace}/bin/launch_bench_nopid     ${BENCH_RUN}  2>&1  | tee ${LOG_DIR}/output.log
}

################################ Main ##################################
BENCH_DRAM=${FAST_TIER_MEMORY} # max memory for node 0 
CONFIG_CXL_MODE=${MEMTIS_CXL_OPTION}
thp_setting=always 

BENCH_DRAM=${FAST_TIER_MEMORY} # max memory for node 0

memtis_userspace=src/memtis_userspace
bin_DIR=${compiled_package_dir}
results_DIR=${output_log_dir}/redis-server-`uname -r`
mkdir -p ${results_DIR}
BENCH_RUN="${redis_dir}/src/redis-server src/testing_scripts/redis/redis.conf"

if [ `uname -r` = "5.15.19-htmm" ];then

    func_prepare
    func_main
else
    ${redis_dir}/src/redis-server src/testing_scripts/redis/redis.conf 
fi

