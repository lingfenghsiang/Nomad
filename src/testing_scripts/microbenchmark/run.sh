#!/bin/bash
# execute this script from project root directory
source global_dirs.sh

result_dir=${output_log_dir}/microbench_nomad
mkdir -p ${result_dir}

${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_zipfan_first_touch_28G.bin -frun=${compiled_package_dir}/all_zipfan_first_touch_28G.bin -fout=${result_dir}/nomad_all_zipfan_first_touch_28G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_zipfan_first_touch_28G.bin -frun=${compiled_package_dir}/all_zipfan_first_touch_28G.bin -fout=${result_dir}/nomad_all_zipfan_first_touch_28G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_zipfan_hottest_28G.bin -frun=${compiled_package_dir}/all_zipfan_hottest_28G.bin -fout=${result_dir}/nomad_all_zipfan_hottest_28G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_zipfan_hottest_28G.bin -frun=${compiled_package_dir}/all_zipfan_hottest_28G.bin -fout=${result_dir}/nomad_all_zipfan_hottest_28G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_seq_28G.bin -frun=${compiled_package_dir}/all_seq_28G.bin -fout=${result_dir}/nomad_all_seq_28G.bin.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_seq_28G.bin -frun=${compiled_package_dir}/all_seq_28G.bin -fout=${result_dir}/nomad_all_seq_28G.bin.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_random_28G.bin  -frun=${compiled_package_dir}/all_random_28G.bin  -fout=${result_dir}/nomad_all_random_28G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/all_random_28G.bin  -frun=${compiled_package_dir}/all_random_28G.bin  -fout=${result_dir}/nomad_all_random_28G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_first_touch_14G.bin -frun=${compiled_package_dir}/run_zipfan_first_touch_14G.bin -fout=${result_dir}/nomad_zipfan_first_touch_14G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_first_touch_14G.bin -frun=${compiled_package_dir}/run_zipfan_first_touch_14G.bin -fout=${result_dir}/nomad_zipfan_first_touch_14G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_14G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_14G.bin -fout=${result_dir}/nomad_zipfan_hottest_14G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_14G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_14G.bin -fout=${result_dir}/nomad_zipfan_hottest_14G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_seq_14G.bin -frun=${compiled_package_dir}/run_seq_14G.bin -fout=${result_dir}/nomad_seq_14G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_seq_14G.bin -frun=${compiled_package_dir}/run_seq_14G.bin -fout=${result_dir}/nomad_seq_14G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_random_14G.bin -frun=${compiled_package_dir}/run_random_14G.bin -fout=${result_dir}/nomad_random_14G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_random_14G.bin -frun=${compiled_package_dir}/run_random_14G.bin -fout=${result_dir}/nomad_random_14G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_first_touch_10G.bin -frun=${compiled_package_dir}/run_zipfan_first_touch_10G.bin -fout=${result_dir}/nomad_zipfan_first_touch_10G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_first_touch_10G.bin -frun=${compiled_package_dir}/run_zipfan_first_touch_10G.bin -fout=${result_dir}/nomad_zipfan_first_touch_10G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_10G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_10G.bin -fout=${result_dir}/nomad_zipfan_hottest_10G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_zipfan_hottest_10G.bin -frun=${compiled_package_dir}/run_zipfan_hottest_10G.bin -fout=${result_dir}/nomad_zipfan_hottest_10G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_seq_10G.bin -frun=${compiled_package_dir}/run_seq_10G.bin -fout=${result_dir}/nomad_seq_10G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_seq_10G.bin -frun=${compiled_package_dir}/run_seq_10G.bin -fout=${result_dir}/nomad_seq_10G.write.log --logtostderr -sleep=5 -work=0
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_random_10G.bin -frun=${compiled_package_dir}/run_random_10G.bin -fout=${result_dir}/nomad_random_10G.read.log --logtostderr -sleep=5 -work=2
${compiled_package_dir}/tpp_mem_access -fwarmup=${compiled_package_dir}/warmup_random_10G.bin -frun=${compiled_package_dir}/run_random_10G.bin -fout=${result_dir}/nomad_random_10G.write.log --logtostderr -sleep=5 -work=0
