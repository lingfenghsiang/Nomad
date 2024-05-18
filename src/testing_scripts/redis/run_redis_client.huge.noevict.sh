#!/bin/bash

source global_dirs.sh


tag=huge
ycsb_dir=third_party/tmp/ycsb-0.17.0

if [ `uname -r` = "5.15.19-htmm" ];then
	output_dir=${output_log_dir}/redis-`uname -r`-${MEMTIS_COOLING_PERIOD}
else
    output_dir=${output_log_dir}/redis-`uname -r`
fi

mkdir -p ${output_dir}
${ycsb_dir}/bin/ycsb load redis -s -P src/testing_scripts/redis/workloada.${tag} -threads 10 -p redis.host=localhost -p redis.port=6379  -p redis.timeout=3600000
echo > ${output_dir}/redis.noevict.log

sleep 10

echo start >> ${output_dir}/redis.noevict.log
cat /proc/vmstat  >> ${output_dir}/redis.noevict.log
${compiled_package_dir}/parse_async_prom -logtostdout >> ${output_dir}/redis.noevict.log

(time ${ycsb_dir}/bin/ycsb run redis -s -P src/testing_scripts/redis/workloada.${tag} -threads 10 -p redis.host=localhost -p redis.port=6379 -p redis.timeout=3600000 >> ${output_dir}/redis.noevict.log
) 2>&1 | tee -a ${output_dir}/redis.noevict.log

echo end >> ${output_dir}/redis.noevict.log
cat /proc/vmstat  >> ${output_dir}/redis.noevict.log
${compiled_package_dir}/parse_async_prom -logtostdout >> ${output_dir}/redis.noevict.log

kill -9 `pgrep redis`
