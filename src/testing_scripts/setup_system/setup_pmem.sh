#!/bin/bash

echo "performance" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

sh -c "echo off > /sys/devices/system/cpu/smt/control"

daxctl reconfigure-device --mode=system-ram all