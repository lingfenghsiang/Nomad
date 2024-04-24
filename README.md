# Nomad

Implementation of paper:

Lingfeng Xiang, Zhen Lin, Weishu Deng, Hui Lu, Jia Rao, Yifan Yuan, Ren Wang. "Nomad: Non-Exclusive Memory Tiering via Transactional Page Migration", OSDI 2024

This repository hosts the code for TPP, Nomad, Memtis, and various testing programs. We recommend compiling our code using Docker, as we have already configured the compilation environment within. You can compile the code on a server with the highest number of CPU cores to expedite the compilation process.

If you want to setup the environment yourself. Please see [here](#setting-up-the-environment-by-yourself).


## Table of Contents

- [Nomad](#nomad)
	- [Table of Contents](#table-of-contents)
	- [Prerequisites](#prerequisites)
		- [Hardware requirements](#hardware-requirements)
		- [Setting up Optane Persistent Memory](#setting-up-optane-persistent-memory)
		- [Setting up CXL memory](#setting-up-cxl-memory)
		- [Compile using docker (Recommended)](#compile-using-docker-recommended)
		- [Setting up the environment by yourself.](#setting-up-the-environment-by-yourself)
		- [Optional](#optional)
	- [Usage](#usage)
		- [Download the code](#download-the-code)
		- [Compiling the code](#compiling-the-code)
	- [Matching paper results](#matching-paper-results)
	- [License](#license)

## Prerequisites

### Hardware requirements

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

### Setting up CXL memory


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

After this step, you will get following compiled files and we need to run them on your platforms:

```
src/tmp/linux-headers-5.13.0-rc6nomad_5.13.0-rc6nomad-nomad_amd64.deb

src/tmp/linux-image-5.13.0-rc6nomad_5.13.0-rc6nomad-nomad_amd64.deb
src/tmp/linux-image-5.13.0-rc6tpp_5.13.0-rc6tpp-tpp_amd64.deb
src/tmp/linux-image-5.15.19-htmm_5.15.19-htmm-memtis_amd64.deb

src/tmp/build/tpp/tpp_mem_access
```

## Matching paper results



## License

[GPLv3](LICENSE)