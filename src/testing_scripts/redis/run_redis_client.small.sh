#!/bin/bash

source global_dirs.sh


tag=small
curr_dir=$PWD
ycsb_dir=${curr_dir}/third_party/tmp/YCSB

if [ `uname -r` = "5.15.19-htmm" ];then
	output_dir=${curr_dir}/${output_log_dir}/redis-${tag}-noevict-`uname -r`-${MEMTIS_COOLING_PERIOD}
else
    output_dir=${curr_dir}/${output_log_dir}/redis-${tag}-noevict-`uname -r`
fi

mkdir -p ${output_dir}
cd ${ycsb_dir}
./bin/ycsb load redis -s -P ${curr_dir}/src/testing_scripts/redis/workloada.${tag} -threads 10 -p redis.host=localhost -p redis.port=6379 
cd ${curr_dir}
echo > ${output_dir}/redis.${tag}.log

sleep 10

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/thrashing-10G.bin \
	-frun=${compiled_package_dir}/thrashing-10G.bin -fout=/tmp/tmp.log -sleep=3 --logtostderr

sleep 10

echo start >> ${output_dir}/redis.${tag}.log
cat /proc/vmstat  >> ${output_dir}/redis.${tag}.log
${compiled_package_dir}/parse_async_prom -logtostdout >> ${output_dir}/redis.${tag}.log

cd ${ycsb_dir}
(time ${ycsb_dir}/bin/ycsb run redis -s -P ${curr_dir}/src/testing_scripts/redis/workloada.${tag} -threads 10 -p redis.host=localhost -p redis.port=6379 >> ${output_dir}/redis.${tag}.log
) 2>&1 | tee -a ${output_dir}/redis.${tag}.log
cd ${curr_dir}

echo end >> ${output_dir}/redis.${tag}.log
cat /proc/vmstat  >> ${output_dir}/redis.${tag}.log
${compiled_package_dir}/parse_async_prom -logtostdout >> ${output_dir}/redis.${tag}.log

kill -9 `pgrep redis`
