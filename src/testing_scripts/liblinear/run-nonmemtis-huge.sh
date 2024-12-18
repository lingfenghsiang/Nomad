#!/bin/bash

 
function func_cache_flush() {
    echo 3 | sudo tee /proc/sys/vm/drop_caches
    free
    return
}
function func_main() {
    
    TIME="/usr/bin/time"
    rm /tmp/liblinear_initialized
    rm /tmp/liblinear_thrashed

    # make directory for/run-tpp results-liblinear
    mkdir -p ${results_DIR}/results-liblinear
    LOG_DIR=${results_DIR}/results-liblinear 

    # flush cache
    func_cache_flush
    sleep 2

	${BENCH_RUN}     2>&1  | tee ${LOG_DIR}/output.log    &

    PID=$(ps | grep 'train' | head -n 1 | awk '{print $1}')

    echo $PID

    while [ ! -e "/tmp/liblinear_initialized" ]
    do
        sleep 1
    done
    
    ${bin_DIR}/tpp_mem_access  -frun=${bin_DIR}/thrashing-15G.bin -anon \
                -fwarmup=${bin_DIR}/thrashing-15G.bin --logtostderr
    echo -----------start----------- > ${LOG_DIR}/memory_status.log
    ${compiled_package_dir}/parse_async_prom -logtostdout >> ${LOG_DIR}/memory_status.log

    echo please run > /tmp/liblinear_thrashed
    wait
    echo -----------finish----------- >> ${LOG_DIR}/memory_status.log
    ${compiled_package_dir}/parse_async_prom -logtostdout >> ${LOG_DIR}/memory_status.log

}
 


################################ Main ##################################
 
# thp_setting=madvise 
source global_dirs.sh
bin_DIR=${compiled_package_dir}
results_DIR=${output_log_dir}/liblinear-huge-`uname -r`
mkdir -p ${results_DIR}
BENCH_BIN=third_party/tmp/liblinear-multicore-2.47
BENCH_RUN="${BENCH_BIN}/train -s 6 -m 80 -e 0.000001  ${BENCH_BIN}/webspam_wc_normalized_trigram.svm"
func_main
