#!/bin/bash

ycsb_dir=/root/ycsb-0.17.0
output_dir=/root/code/src/testing_scripts/microbenchmark/tmp
script_dir=/root/code/src/testing_scripts/microbenchmark

mkdir -p ${output_dir}

func1(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-13.5G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_13.5G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-13.5G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_13.5G.bin
}

func2(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-13.5G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_13.5G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-13.5G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_13.5G.bin
}

func3(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-10G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_10G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-10G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_10G.bin
}

func4(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-10G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_10G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-10G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_10G.bin
}

func5(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-18G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_18G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-18G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_18G.bin
}

func6(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-18G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_18G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-18G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_18G.bin
}

func7(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-27G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_27G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-27G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_27G.bin
}

func8(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-27G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/warmup_zipfan_first_touch_27G.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-27G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -o $offset -f ${output_dir}/run_zipfan_first_touch_27G.bin
}

func9(){
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/robustness-23G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/robustness-23G.bin
}

func10(){
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/robustness-25G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/robustness-25G.bin
}

func11(){
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/robustness-27G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/robustness-27G.bin
}

func12(){
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/robustness-29G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/robustness-29G.bin
}

func13(){
    python3 ${script_dir}/create_thrashing.py -f ${output_dir}/thrashing-15G.bin -s 15
    python3 ${script_dir}/create_thrashing.py -f ${output_dir}/thrashing-10G.bin -s 10
}

func14(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-10block  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_10block.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-10block  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_10block.bin
}

func15(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-13block  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_13block.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-13block  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_13block.bin
}

func16(){
    offset=`${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-27block  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/warmup_zipfan_hottest_27block.bin`
    ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-27block  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -o $offset -f ${output_dir}/run_zipfan_hottest_27block.bin
}

func17(){
   ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-24G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 0 -f ${output_dir}/zipfan_hottest-24G.bin
   ${ycsb_dir}/bin/ycsb.sh run basic -P ${script_dir}/workloada-24G  | python3 ${script_dir}/filter_access_pattern.py | python3 ${script_dir}/write_binary_data.py -p 1 -f ${output_dir}/zipfan_firsttouch-24G.bin
}

func1 &
func2 &
func3 &
func4 &
func5 &
func6 &
func7 &
func8 &
func9 &
func10 &
func11 &
func12 &
func13 &
func14 &
func15 &
func16 &
func17 &
wait
echo "pattern files generated"