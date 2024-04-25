#!/bin/bash
# execute this script from project root directory
echo 1 >/sys/kernel/mm/numa/demotion_enabled
echo 2 >/proc/sys/kernel/numa_balancing
swapoff -a
echo 1000 >/proc/sys/vm/demote_scale_factor