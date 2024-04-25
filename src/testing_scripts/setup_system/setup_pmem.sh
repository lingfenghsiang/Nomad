#!/bin/bash

sh -c "echo off > /sys/devices/system/cpu/smt/control"

echo "performance" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

daxctl reconfigure-device --mode=system-ram all