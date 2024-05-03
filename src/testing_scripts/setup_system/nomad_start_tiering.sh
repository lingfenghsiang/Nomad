#!/bin/bash
# execute this script from project root directory
original_dir=$PWD
cd src/nomad_module && make clean && make && insmod async_promote.ko

echo 1 >/sys/kernel/mm/numa/demotion_enabled
echo 2 >/proc/sys/kernel/numa_balancing
swapoff -a
echo 2200 >/proc/sys/vm/demote_scale_factor
cd ${original_dir}
