#!/bin/bash
# execute this script from project root directory
source global_dirs.sh

result_dir=${output_log_dir}/motivation_`uname -r`
mkdir -p ${result_dir}


${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_first_touch_10G.bin -frun=${compiled_package_dir}/run_zipfan_first_touch_10G.bin -fout=${result_dir}/zipfan_first_touch_10G.write.log --logtostderr -sleep=10 -work=0
sleep 20

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_first_touch_27G.bin -frun=${compiled_package_dir}/warmup_zipfan_first_touch_27G.bin -fout=${result_dir}/zipfan_first_touch_27G.write.log --logtostderr -sleep=10 -work=0
sleep 20

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_10G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_10G.bin -fout=${result_dir}/zipfan_hottest_10G.write.log --logtostderr -sleep=10 -work=0
sleep 20

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_27G.bin -frun=${compiled_package_dir}/warmup_zipfan_hottest_27G.bin -fout=${result_dir}/zipfan_hottest_27G.write.log --logtostderr -sleep=10 -work=0
sleep 20
