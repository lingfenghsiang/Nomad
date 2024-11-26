#!/bin/bash

 
###### update DIR!
 
 

function func_cache_flush() {
    echo 3 | sudo tee /proc/sys/vm/drop_caches
    free
    return
}

function func_main() {
 
    TIME="/usr/bin/time"

    # make directory for run-tpp/results-pr

    
    mkdir -p ${results_DIR}/results-pr
    LOG_DIR=${results_DIR}/results-pr 
 
    # flush cache
    func_cache_flush
    sleep 2

    echo -----------start----------- > ${LOG_DIR}/memory_status.log
    ${compiled_package_dir}/parse_async_prom -logtostdout >> ${LOG_DIR}/memory_status.log

	${TIME} -f "execution time %e (s)" \
    ${BENCH_RUN}  2>&1 | tee ${LOG_DIR}/output.log  
    echo -----------finish----------- >> ${LOG_DIR}/memory_status.log
    ${compiled_package_dir}/parse_async_prom -logtostdout >> ${LOG_DIR}/memory_status.log

}

################################ Main ##################################
# thp_setting=madvise 
 
source global_dirs.sh
bin_DIR=${compiled_package_dir}
results_DIR=${output_log_dir}/pageranking-huge-`uname -r`
mkdir -p ${results_DIR}
BENCH_BIN=third_party/tmp/gapbs
BENCH_RUN="${BENCH_BIN}/pr  -u28 -k20 -i10 -n100"

func_main
