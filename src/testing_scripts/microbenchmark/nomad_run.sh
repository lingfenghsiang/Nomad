#!/bin/bash
# execute this script from project root directory
source global_dirs.sh

result_dir=${output_log_dir}/microbench_nomad
mkdir -p ${result_dir}


${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_10G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_10G.bin -fout=${result_dir}/zipfan_hottest_10G.read.log --logtostderr -sleep=10 -work=2
sleep 20
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_10G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_10G.bin -fout=${result_dir}/zipfan_hottest_10G.write.log --logtostderr -sleep=10 -work=0
sleep 20

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_13.5G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_13.5G.bin -fout=${result_dir}/zipfan_hottest_13.5G.read.log --logtostderr -sleep=10 -work=2
sleep 20
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_13.5G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_13.5G.bin -fout=${result_dir}/zipfan_hottest_13.5G.write.log --logtostderr -sleep=10 -work=0
sleep 20

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_27G.bin -frun=${compiled_package_dir}/warmup_zipfan_hottest_27G.bin -fout=${result_dir}/zipfan_hottest_27G.read.log --logtostderr -sleep=10 -work=2
sleep 20
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_27G.bin -frun=${compiled_package_dir}/warmup_zipfan_hottest_27G.bin -fout=${result_dir}/zipfan_hottest_27G.write.log --logtostderr -sleep=10 -work=0
sleep 20



${compiled_package_dir}/trigger_llc_miss_access -pattern ${compiled_package_dir}/warmup_zipfan_hottest_10block.bin,${compiled_package_dir}/run_zipfan_hottest_10block.bin,${compiled_package_dir}/run_zipfan_hottest_10block.bin,${compiled_package_dir}/run_zipfan_hottest_10block.bin,${compiled_package_dir}/run_zipfan_hottest_10block.bin --blocksz=1g --output=${result_dir}/10plus10block.log --logtostderr -interval=10
sleep 20

${compiled_package_dir}/trigger_llc_miss_access -pattern ${compiled_package_dir}/warmup_zipfan_hottest_13block.bin,${compiled_package_dir}/run_zipfan_hottest_13block.bin,${compiled_package_dir}/run_zipfan_hottest_13block.bin,${compiled_package_dir}/run_zipfan_hottest_13block.bin,${compiled_package_dir}/run_zipfan_hottest_13block.bin --blocksz=1g --output=${result_dir}/13plus13block.log --logtostderr -interval=10
sleep 20

${compiled_package_dir}/trigger_llc_miss_access -pattern ${compiled_package_dir}/warmup_zipfan_hottest_27block.bin,${compiled_package_dir}/warmup_zipfan_hottest_27block.bin,${compiled_package_dir}/warmup_zipfan_hottest_27block.bin,${compiled_package_dir}/warmup_zipfan_hottest_27block.bin,${compiled_package_dir}/warmup_zipfan_hottest_27block.bin --blocksz=1g --output=${result_dir}/27block.log --logtostderr -interval=10
sleep 20
