#!/bin/bash
# execute this script from project root directory

curr_dir=$PWD
cd src/memtis_userspace && make
cd ${curr_dir}
swapoff -a