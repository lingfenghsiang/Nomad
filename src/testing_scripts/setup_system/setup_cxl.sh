#!/bin/bash
# execute this script from project root directory

# nothing much todo, just turn off hyperthreading and tune CPU to best performance
echo "performance" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

sh -c "echo off > /sys/devices/system/cpu/smt/control"
