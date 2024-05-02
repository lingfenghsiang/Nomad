#!/bin/bash
# execute this script from project root directory
source global_dirs.sh

if [ `uname -r` != "5.13.0-rc6nomad" ];then
echo please run nomad!
exit 1
fi

result_dir=${output_log_dir}/robustness_nomad
mkdir -p ${result_dir}

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/robustness-23G.bin -frun=${compiled_package_dir}/robustness-23G.bin -fout=${result_dir}/robustness-23G.log --logtostderr -sleep=5 -work=2
sleep 20
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/robustness-25G.bin -frun=${compiled_package_dir}/robustness-25G.bin -fout=${result_dir}/robustness-25G.log --logtostderr -sleep=5 -work=2
sleep 20
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/robustness-27G.bin -frun=${compiled_package_dir}/robustness-27G.bin -fout=${result_dir}/robustness-27G.log --logtostderr -sleep=5 -work=2
sleep 20
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/robustness-29G.bin -frun=${compiled_package_dir}/robustness-29G.bin -fout=${result_dir}/robustness-29G.log --logtostderr -sleep=5 -work=2
sleep 20
