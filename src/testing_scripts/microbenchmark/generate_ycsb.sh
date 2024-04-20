#!/bin/bash

ycsb_dir=/root/ycsb-0.17.0
output_dir=/root/code/src/testing_scripts/microbenchmark/tmp

mkdir -p ${output_dir}

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-13.5G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_13.5G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-13.5G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_13.5G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-13.5G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_13.5G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-13.5G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_13.5G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-10G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_10G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-10G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_10G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-10G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_10G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-10G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_10G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-18G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_18G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-18G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_18G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-18G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_18G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-18G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_18G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-27G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_27G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-27G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_27G.bin

offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-27G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_27G.bin`
${ycsb_dir}/bin/ycsb.sh run basic -P ./workloada-27G  | python3 filter_access_pattern.py | python3 write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_27G.bin
