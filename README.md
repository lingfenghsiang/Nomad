# Nomad

Implementation of paper:

Lingfeng Xiang, Zhen Lin, Weishu Deng, Hui Lu, Jia Rao, Yifan Yuan, Ren Wang. "Nomad: Non-Exclusive Memory Tiering via Transactional Page Migration", OSDI 2024

This repository hosts the code for TPP, Nomad, Memtis, and various testing programs. We recommend compiling our code using Docker, as we have already configured the compilation environment within. You can compile the code on a server with the highest number of CPU cores to expedite the compilation process.

If you want to setup the environment yourself. Please see [here](#setting-up-the-environment-by-yourself).


## Table of Contents

- [Nomad](#nomad)
	- [Table of Contents](#table-of-contents)
	- [Prerequisites](#prerequisites)
		- [Software requirements](#software-requirements)
		- [Hardware requirements](#hardware-requirements)
		- [Setting up Optane Persistent Memory](#setting-up-optane-persistent-memory)
		- [Setting up CXL memory](#setting-up-cxl-memory)
		- [Compile using docker (Recommended)](#compile-using-docker-recommended)
		- [Setting up the environment by yourself.](#setting-up-the-environment-by-yourself)
		- [Optional](#optional)
	- [Usage](#usage)
		- [Download the code](#download-the-code)
		- [Compiling the code](#compiling-the-code)
	- [Reproducing paper results](#reproducing-paper-results)
		- [Steps to take:](#steps-to-take)
		- [Matching paper results](#matching-paper-results)
	- [License](#license)

## Prerequisites

### Software requirements

To compile our Nomad module, you also need to install `gcc`, `make`, `pkg-config`, `time`, `python2`, `openjdk-8-jre` on the running server.


On persistent memory server you need to install 

```
sudo apt install -y ndctl ipmctl
```


### Hardware requirements

**Compiling code**
You'll need a minimum of 30GB of disk space and 16GB of memory. The machine used for compilation doesn't necessarily have to be the same one where the code will run. Utilize as many CPUs as are available for compilation, as it can be time-consuming.


**Running code**
At present, our system requires one CPU with a memory NUMA node and one CPU-less NUMA memory node. We do not support multiple CPU NUMA nodes. If your system has more than one CPU NUMA node, you can disable the others and leave only one CPU node enabled in the BIOS settings. As for the memory NUMA node, you can use Persistent Memory, CXL Memory, or virtualize such configurations to run our code in virtual machines.

A typical hardware configuration appears as below:
```
# numactl -H
available: 2 nodes (0-1)
node 0 cpus: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
node 0 size: 15264 MB
node 0 free: 12065 MB
node 1 cpus:
node 1 size: 16127 MB
node 1 free: 15924 MB
node distances:
node   0   1
  0:  10  14
  1:  14  10
```

### Setting up Optane Persistent Memory

Reconfigure persistent memory hardware.
```
# destroy namespaces on persistent memory
sudo ndctl destroy-namespace -f all
# reconfigure persistent memory
sudo ipmctl create  -goal
# restart machine to make new configuration available
sudo reboot
```
Set up Persistent Memory namespace as root user.
```
su
ndctl create-namespace --mode=devdax --map=mem
daxctl migrate-device-model
set +o noclobber
echo offline > /sys/devices/system/memory/auto_online_blocks
```
Set Persistent Memory as volatile system memory.
```
daxctl reconfigure-device --mode=system-ram all
```

### Setting up CXL memory

Add the following kernel parameter to utilize CXL.mem device in file `/etc/default/grub`.

```
GRUB_CMDLINE_LINUX_DEFAULT="efi=nosoftreserve"
```
Then update the grub by executing:
```
sudo update-grub2
```

Then reboot your machine.


### Compile using docker (Recommended)

We offer Docker environments for compiling both the kernel and userspace programs. The compilation process is static, meaning you only need to compile once on a single machine, and then you can transfer the compiled program to the server.

Install docker:
```
curl https://get.docker.com/ | bash
```

After this step, you may jump to section [Compiling](#compiling-the-code).

### Setting up the environment by yourself.

You need to execute the following command to compile kernel. We compiled our code on Ubuntu 20.04LTS. If you compile on Ubuntu 22.04, you may encouter tool chain problems.
```
apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison dwarves zstd
```
You also need to install `cmake`, `gflags`, `glog` and `python` to compile our userspace program and generate the access pattern file.

### Optional
If you want to try Nomad or TPP in a virtual machine, you may need network access to the virtual machine.
```
sudo apt-get install qemu-kvm libvirt-daemon-system libvirt-clients bridge-utils
sudo adduser `id -un` libvirt
sudo adduser `id -un` kvm
```

## Usage

This section focuses on how to compile our code, install the packed kernel, and how to run our code.


### Download the code
```
git clone https://github.com/lingfenghsiang/Nomad.git
cd Nomad
```

### Compiling the code

To compile our code, you need to execute:

```
sudo bash compile.sh
```

Estimated compile time on a 96 core E5-4640 server: 1hour

After this step, you will get following compiled files under directory `src/tmp/output/` and you may directly send the folder `src/tmp/output/` to your testing platform:

```
# ls src/tmp/output/
linux-headers-5.13.0-rc6nomad_5.13.0-rc6nomad-nomad_amd64.deb
linux-headers-5.13.0-rc6tpp_5.13.0-rc6tpp-tpp_amd64.deb
linux-headers-5.15.19-htmm_5.15.19-htmm-memtis_amd64.deb
linux-image-5.13.0-rc6nomad_5.13.0-rc6nomad-nomad_amd64.deb
linux-image-5.13.0-rc6nomad-dbg_5.13.0-rc6nomad-nomad_amd64.deb
linux-image-5.13.0-rc6tpp_5.13.0-rc6tpp-tpp_amd64.deb
linux-image-5.13.0-rc6tpp-dbg_5.13.0-rc6tpp-tpp_amd64.deb
linux-image-5.15.19-htmm_5.15.19-htmm-memtis_amd64.deb
linux-image-5.15.19-htmm-dbg_5.15.19-htmm-memtis_amd64.deb
linux-libc-dev_5.13.0-rc6nomad-nomad_amd64.deb
linux-libc-dev_5.13.0-rc6tpp-tpp_amd64.deb
linux-libc-dev_5.15.19-htmm-memtis_amd64.deb
run_zipfan_first_touch_10G.bin
run_zipfan_first_touch_13.5G.bin
run_zipfan_first_touch_18G.bin
run_zipfan_first_touch_27G.bin
run_zipfan_hottest_10G.bin
run_zipfan_hottest_13.5G.bin
run_zipfan_hottest_18G.bin
run_zipfan_hottest_27G.bin
tpp_mem_access
warmup_zipfan_first_touch_10G.bin
warmup_zipfan_first_touch_13.5G.bin
warmup_zipfan_first_touch_18G.bin
warmup_zipfan_first_touch_27G.bin
warmup_zipfan_hottest_10G.bin
warmup_zipfan_hottest_13.5G.bin
warmup_zipfan_hottest_18G.bin
warmup_zipfan_hottest_27G.bin
```

## Reproducing paper results

### Steps to take:
1. Compile the code on a machine that has a lot of cores (Do this only once).
2. Install kernels (Do this on each machine you need to run on).
3. Send compiled binary package to the running server
4. Compile the applications
   ```
   bash third_party/prepare.sh
   ```
5. Setting up the machine
   * If it's a pmem server:
		```
		sudo bash src/testing_scripts/setup_system/setup_pmem.sh
		```
   * If it's a CXL server:
		```
		sudo bash src/testing_scripts/setup_system/setup_cxl.sh
		```
   * bash src/testing_scripts/setup_system/memtis_prepare.sh
6. Setting up the OS:
   * If it's TPP:
		```
		sudo bash src/testing_scripts/setup_system/tpp_start_tiering.sh
		```
	* If it's Nomad:
		```
		sudo bash src/testing_scripts/setup_system/nomad_start_tiering.sh
		sudo bash src/testing_scripts/pageranking/run-nomad.sh
		sudo bash src/testing_scripts/liblinear/run-nomad.sh
		```
	* If it's Memtis:
		```
		sudo bash src/testing_scripts/setup_system/memtis_prepare.sh
		```
sudo ln -s /usr/bin/python3 /usr/bin/python
### Matching paper results

## License

[GPLv3](LICENSE)