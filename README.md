# Nomad

Implementation of paper:

Lingfeng Xiang, Zhen Lin, Weishu Deng, Hui Lu, Jia Rao, Yifan Yuan, Ren Wang. "Nomad: Non-Exclusive Memory Tiering via Transactional Page Migration", OSDI 2024

This repository hosts the code for TPP, Nomad, Memtis, and various testing programs. We recommend compiling our code using Docker, as we have already configured the compilation environment within. You can compile the code on a server with the highest number of CPU cores to expedite the compilation process.

If you want to setup the environment yourself, please see [here](#setting-up-the-environment-by-yourself).


## Table of Contents

- [Nomad](#nomad)
	- [Table of Contents](#table-of-contents)
	- [Note](#note)
	- [Prerequisites](#prerequisites)
		- [Software requirements](#software-requirements)
		- [Hardware requirements](#hardware-requirements)
		- [Setting up Optane Persistent Memory as system memory](#setting-up-optane-persistent-memory-as-system-memory)
		- [Setting up CXL memory](#setting-up-cxl-memory)
		- [Compile using docker (Recommended)](#compile-using-docker-recommended)
		- [Setting up the environment by yourself.](#setting-up-the-environment-by-yourself)
		- [Optional](#optional)
	- [Usage](#usage)
		- [Download the code](#download-the-code)
		- [Compiling the code](#compiling-the-code)
		- [Switch a kernel](#switch-a-kernel)
	- [Reproducing paper results](#reproducing-paper-results)
		- [Steps to take](#steps-to-take)
		- [Matching paper results](#matching-paper-results)
			- [Figure 7](#figure-7)
			- [Figure 8](#figure-8)
			- [Table 2](#table-2)
			- [Table 3](#table-3)
			- [Table 4](#table-4)
			- [Figure 9](#figure-9)
			- [Figure 10](#figure-10)
			- [Figure 11](#figure-11)
			- [Figure 12](#figure-12)
	- [License](#license)

## Note

All the shell scripts should be run at this project root directory. Don't `cd` any folders to execute our scripts.

## Prerequisites

### Software requirements

**Operating System**
We tested our code on both Ubuntu 22.04 and Debian 11. You may face technical issues when setting Persistent Memory on Ubuntu. It's recommended to install Debian (We tested 11, but didn't try Debian 12) on Persistent memory server to avoid such problems. Our compiling scripts only compile kernel packages for Debian-based GNU/Linux (like Ubuntu, Debian, Linux Mint etc.). If you run on Red Hat based OS (like CentOS, Fedora, RHEL), please search instructions about `rpm` kernel compiling options and update the compiling scripts.

**Dependencies**
To compile our Nomad module and userspace applications, you also need to:
```
sudo apt install gcc g++ make pkg-config time python2 openjdk-11-jre rsync unzip
```
When running Redis, we require YCSB, which requires `python` to be `python2`. In that case you need to create a soft link as below:
```
sudo ln -s /usr/bin/python2 /usr/bin/python
```

To configure persistent memory you need to:

```
sudo apt install -y ndctl ipmctl
```

To plot the graphs, you need python3 and the following dependencies installed:
```
sudo apt install python3-pip
pip3 install matplotlib pandas numpy json5
```

### Hardware requirements

**For code compilation**
You'll need a minimum of 60GB of disk space and 16GB of memory. The machine used for compilation doesn't necessarily have to be the same one where the code will run. Utilize as many CPUs as possible for compilation, as it can be time-consuming.


**Running code**
At present, our system requires one CPU with a memory NUMA node and one CPU-less NUMA memory node. We do not support multiple CPU NUMA nodes. If your system has more than one CPU NUMA node, you can disable the others and leave only one CPU node enabled in the BIOS settings. As for the memory NUMA node, you can use Persistent Memory, CXL Memory (we use Intel Agilex in the paper), or virtualize such configurations to run our code in virtual machines.

A typical hardware configuration appears as below:
```
# numactl -H
available: 2 nodes (0-1)
node 0 cpus: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
node 0 size: 15264 MB
node 0 free: 13707 MB
node 1 cpus:
node 1 size: 16127 MB
node 1 free: 16017 MB
node distances:
node   0   1
  0:  10  14
  1:  14  10
```

The hardware memory size may greatly influence the performance of microbenchmarks, especially for [medium working set size case](#figure-8). For medium size microbenchmark, you need to have free memory almost equivalent to the RSS (13.5GB in the case of our microbenchmark). A slightly larger local memory will turn it into the small working set size case where the whole RSS is accommodated in local DRAM.

To set local DRAM sizes, you may add `GRUB_CMDLINE_LINUX="memmap=nn[KMG]!ss[KMG]"` in your `/etc/default/grub` file. For more details, you may check [this link](https://pmem.io/blog/2016/02/how-to-emulate-persistent-memory/).

When tuning the local memory size, you may need pay attention to the space overhead for `struct page`. For instance, let's say you have 32GB of local DRAM and 512GB of persistent memory. Each 4KB persistent memory requires a 64-byte `struct page` on local DRAM, which results in 8GB `struct page` on local DRAM allocated, even though no application is running.



### Setting up Optane Persistent Memory as system memory

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

In the folder `src/vm_scripts`, you'll find some scripts designed to configure a virtual machine with a CPU-less memory node. Feel free to explore them further.

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
ls -1 src/tmp/output/
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
parse_async_prom
robustness-23G.bin
robustness-25G.bin
robustness-27G.bin
robustness-29G.bin
run_zipfan_first_touch_10G.bin
run_zipfan_first_touch_13.5G.bin
run_zipfan_first_touch_18G.bin
run_zipfan_first_touch_27G.bin
run_zipfan_hottest_10G.bin
run_zipfan_hottest_13.5G.bin
run_zipfan_hottest_18G.bin
run_zipfan_hottest_27G.bin
thrashing-10G.bin
thrashing-15G.bin
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

### Switch a kernel
If you need to switch a kernel version, please use the following script and follow the prompts in the terminal.
```
sudo bash src/testing_scripts/setup_system/switch_kernel.sh
```
Then you need to:
```
sudo reboot
```

## Reproducing paper results

### Steps to take
1. **On compiling machine**. Compile the code on a machine that has a lot of cores (Do this only once on only one machine. It takes up to 1 hour or more). See [Compiling the code](#compiling-the-code).
   ```
   sudo bash compile.sh
   ```

2. **On compiling machine**. Send compiled binary package to the running server. You may use rsync, sftp, scp to send the whole directory:

   e.g. If you use `rsync`, you need to install `rsync` on both the sender and receiver machine. Don't forget to replace your user name, machine IP and the folder address.
   ```
   rsync -av src/tmp/output user_name@machine_ip:/home/foobar/Downloads
   ```

3. **On testing (CXL/PMem) machine**. Setting up the environment.
   The global directory is set in file `global_dirs.sh`.
	`compiled_package_dir` is the compiled kernel and access pattern files, it's the directory `/home/foobar/Downloads/output` in Step 2, if you send output to the aforementioned example directory.
	`output_log_dir` is the directory that contains the results.
	`MEMTIS_CXL_OPTION` is an option for Memtis. This option is used to select event and determine memory node. We already hard coded the event and the memory node. Please always set it to `on`, whether or not it's a CXL machine.
   ```
	compiled_package_dir=src/tmp/output
	output_log_dir=src/tmp/results

	MEMTIS_CXL_OPTION=on
   ```
4. **On testing (CXL/PMem) machine**. Install kernels (Do this on each machine you need to run on).
   ```
   cd Nomad
   sudo bash install.sh
   ```

5.  **On testing (CXL/PMem) machine**. Compile the applications
	```
	bash third_party/prepare.sh
	```

6. **On testing (CXL/PMem) machine**. Switch to a kernel you want to test (see section [Switch a kernel](#switch-a-kernel)) and reboot. The available kernel version may look like:
	```
	Available kernel versions:
	5.10.0-28-amd64 # original kernel
	5.13.0-rc6nomad # nomad kernel
	5.13.0-rc6tpp	# tpp kernel
	5.15.19-htmm	# memtis kernel
	```


7.  **On testing (CXL/PMem) machine**. Setting up the machine (Do this everytime when you start the machine):
   * If it's a pmem server:
		```
		sudo bash src/testing_scripts/setup_system/setup_pmem.sh
		```
   * If it's a CXL server:
		```
		sudo bash src/testing_scripts/setup_system/setup_cxl.sh
		```

8.  **On testing (CXL/PMem) machine**. Setting up the OS (Do this everytime when you start the machine):
    * If it's TPP:
    	```
    	sudo bash src/testing_scripts/setup_system/tpp_start_tiering.sh
    	```

    * If it's Nomad:
    	```
    	sudo bash src/testing_scripts/setup_system/nomad_start_tiering.sh
    	```
    * If it's Memtis:
    	```
    	sudo bash src/testing_scripts/setup_system/memtis_prepare.sh
    	```
9.  **On testing (CXL/PMem) machine**. Run microbenchmark.  30-45minutes. (Run Nomad, TPP, Memtis. If it fails for Nomad and TPP, please restart the machine, go over step 7 and 8 and do this step.)
   
	```
	sudo bash src/testing_scripts/microbenchmark/run.sh
	```
10. **On testing (CXL/PMem) machine**. Run Redis. Less than 1 hour. (Run Nomad and TPP. Don't run Redis on Memtis, it will fail. If it fails for Nomad and TPP, please restart the machine, go over step 7 and 8 and do this step.)
		
	```
	sudo bash src/testing_scripts/redis/run_redis.sh
	```
	If you encounter issues like `java.net.SocketTimeoutException: Read timed out`, you may need to run the each cases individually:
	```
	sudo bash src/testing_scripts/redis/run_redis.sh -r 1
	sudo bash src/testing_scripts/redis/run_redis.sh -r 2
	sudo bash src/testing_scripts/redis/run_redis.sh -r 3
	```

11. **On testing (CXL/PMem) machine**. Run PageRanking. 30-45minutes. (Run Nomad, TPP, Memtis, and an original kernel, the very first kernel when the OS was installed. If it fails for Nomad and TPP, please restart the machine, go over step 7 and 8 and do this step.)
		
	```
	sudo bash src/testing_scripts/pageranking/run.sh
	```
12. **On testing (CXL/PMem) machine**. Run Liblinear. 30-45minutes. (Run Nomad, TPP, Memtis, and an original kernel, the very first kernel when the OS was installed. If it fails for Nomad and TPP, please restart the machine, go over step 7 and 8 and do this step.)
	
	```
	sudo bash src/testing_scripts/liblinear/run.sh
	```
13. **On testing (CXL/PMem) machine**. If you need to test a different kernel, go to step 6. Otherwise, you are done with running the tests.

14. **On testing (CXL/PMem) machine**. Run robustness test. 15 minutes. This is for **Nomad only** and the hardware configuration should be 16GB local DRAM + 16GB slow memory (CXL/PMem).
	```
	sudo bash src/testing_scripts/microbenchmark/robustness.sh
	```

15. **On testing (CXL/PMem) machine**. When you finish running all the tests, run the following command to generate plots. To plot the graphs, you need to install some python packages, please see section [Software requirements](#software-requirements).
    ```
	bash src/post_processing/plot_graphs.sh
	```

### Matching paper results

After you run the test, you will find all the result logs in directory `src/tmp/results`.

#### Figure 7

| Figure info        | Microbenchmark for small working set size                                                                                                                                                                                                                                                                                                                                                       |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Raw data directory | `src/tmp/results/microbench_memtis/zipfan_hottest_10G.read.log`, `src/tmp/results/microbench_memtis/zipfan_hottest_10G.write.log`, `src/tmp/results/microbench_nomad/zipfan_hottest_10G.read.log`, `src/tmp/results/microbench_nomad/zipfan_hottest_10G.write.log`, `src/tmp/results/microbench_tpp/zipfan_hottest_10G.read.log`, `src/tmp/results/microbench_tpp/zipfan_hottest_10G.write.log` |
| How to interpret   | In each log file, you will find the log for five rounds. In each round, you can find a line including `[note]:[number]`, indicating which round it is. The first round (`[note]:[0]`) is for warming up, the second round (`[note]:[1]`) is for "migration in process" and the last round (`[note]:[4]`) is for "migration stable" in the paper.                                                |
| Plot path | `src/post_processing/tmp/microbenchsmall-read.png`, `src/post_processing/tmp/microbenchsmall-write.png` |
#### Figure 8


| Figure info        | Microbenchmark for medium working set size                                                                                                                                                                                                                                                                                                                                                                  |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Raw data directory | `src/tmp/results/microbench_memtis/zipfan_hottest_13.5G.read.log`, `src/tmp/results/microbench_memtis/zipfan_hottest_13.5G.write.log`, `src/tmp/results/microbench_nomad/zipfan_hottest_13.5G.read.log`, `src/tmp/results/microbench_nomad/zipfan_hottest_13.5G.write.log`, `src/tmp/results/microbench_tpp/zipfan_hottest_13.5G.read.log`, `src/tmp/results/microbench_tpp/zipfan_hottest_13.5G.write.log` |
| How to interpret   | In each log file, you will find the log for five rounds. In each round, you can find a line including `[note]:[number]`, indicating which round it is. The first round (`[note]:[0]`) is for warming up, the second round (`[note]:[1]`) is for "migration in process" and the last round (`[note]:[4]`) is for "migration stable" in the paper.                                                            |
| Plot path | `src/post_processing/tmp/microbenchmedium-read.png`, `src/post_processing/tmp/microbenchmedium-write.png` |
| Note | If you find that TPP hasn't migrated pages in the last few rounds, it's probable that the fast tier has already accommodated the RSS. Please take a look at section [Hardware requirements](#hardware-requirements) |

#### Table 2

| Table info                              | Number of promotions                                                                                                                                         |
| --------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Raw data                                | All the log files for figure 7, figure 8 and figure 9.                                                                                                       |
| How to calculate Nomad promotion number | Calculate `end_success_nr` - `start_success_nr` using the data from Nomad logs. The log path is mentioned in info for figure 7, figure 8 and figure 9.       |
| How to calculate Nomad demotion number  | Calculate `end_pgdemote_anon` - `start_pgdemote_anon` using the data from Nomad logs. The log path is mentioned in info for figure 7, figure 8 and figure 9. |
| How to calculate TPP promotion number   | Calculate `end_pgpromote_anon` - `start_pgpromote_anon` using the data from TPP logs. The log path is mentioned in info for figure 7, figure 8 and figure 9. |
| How to calculate TPP demotion number    | Calculate  `end_pgdemote_anon` - `start_pgdemote_anon`  using the data from TPP logs. The log path is mentioned in info for figure 7, figure 8 and figure 9. |


#### Table 3

| Table info                          | The success rate of Nomad                                                                                                                                        |
| ----------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Raw data                            | All the Nomad log files for figure 7, figure 8 and figure 9                                                                                                      |
| How to calculate Nomad success rate | `Success number` is `end_success_nr` - `start_success_nr`. `Abort number` is `end_retry_nr` - `start_retry_nr`. Success rate is `Success number`: `Abort number` |

#### Table 4

| Table info                          | Robustness |
| ----------------------------------- | ----------------------------------- | 
| Raw data |  `src/tmp/results/robustness_nomad/robustness-23G.log`, `src/tmp/results/robustness_nomad/robustness-25G.log`, `src/tmp/results/robustness_nomad/robustness-27G.log`, `src/tmp/results/robustness_nomad/robustness-29G.log` |
| How to interpret |  Each round of test will have `[end_shadow_page_pair]:[30000]`. The number `30000` means there are 30000 shadow pages linked to fast tier pages when the tests are over. Please always check the last round of the test. |


#### Figure 9


| Figure info        | Microbenchmark for large working set size                                                                                                                                                                                                                                                                                                                                                       |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Raw data directory | `src/tmp/results/microbench_memtis/zipfan_hottest_27G.read.log`, `src/tmp/results/microbench_memtis/zipfan_hottest_27G.write.log`, `src/tmp/results/microbench_nomad/zipfan_hottest_27G.read.log`, `src/tmp/results/microbench_nomad/zipfan_hottest_27G.write.log`, `src/tmp/results/microbench_tpp/zipfan_hottest_27G.read.log`, `src/tmp/results/microbench_tpp/zipfan_hottest_27G.write.log` |
| How to interpret   | In each log file, you will find the log for five rounds. In each round, you can find a line including `[note]:[number]`, indicating which round it is. The first round (`[note]:[0]`) is for warming up, the second round (`[note]:[1]`) is for "migration in process" and the last round (`[note]:[4]`) is for "migration stable" in the paper.                                                |
| Plot path | `src/post_processing/tmp/microbenchlarge-read.png`, `src/post_processing/tmp/microbenchlarge-write.png` |
#### Figure 10

| Figure info        | Running YCSB on redis                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Raw data directory | `src/tmp/results/redis-5.13.0-rc6nomad/redis.large.log`, `src/tmp/results/redis-5.13.0-rc6nomad/redis.noevict.log`, `src/tmp/results/redis-5.13.0-rc6nomad/redis.small.log`, `src/tmp/results/redis-5.13.0-rc6tpp/redis.large.log`, `src/tmp/results/redis-5.13.0-rc6tpp/redis.noevict.log`, `src/tmp/results/redis-5.13.0-rc6tpp/redis.small.log`, `src/tmp/results/redis-5.15.19-htmm/redis.large.log`, `src/tmp/results/redis-5.15.19-htmm/redis.noevict.log`, `src/tmp/results/redis-5.15.19-htmm/redis.small.log` |
| How to interpret   | In each log, you will find a line like `[OVERALL], Throughput(ops/sec), 49385.76`, this indicates the throughput. For each kernel, `redis.small.log` corresponds to Case 1, `redis.large.log` corresponds to Case 2, and  `redis.noevict.log` corresponds to Case 3                                                                                                                                                                                                                                                    |
| Plot path | `src/post_processing/tmp/redis.png` |

#### Figure 11

| Figure info        | Results for PageRank                                                                                                                                                                                                                  |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Raw data directory | `src/tmp/results/pageranking-5.13.0-rc6nomad/results-pr/output.log`, `src/tmp/results/pageranking-5.13.0-rc6tpp/results-pr/output.log`, `src/tmp/results/pageranking-5.15.19-htmm/results-pr/output.log`                              |
| How to interpret   | For each log file, you will find a line  like `execution time 1690.20 (s)`. Then its speed is `1/1690.2`. When you compare across multiple kernel and platforms, divide the speed by the slowest one, then you get data in figure 11. |
| Plot path | `src/post_processing/tmp/pageranking.png` |

#### Figure 12

| Figure info        | Results for Liblinear                                                                                                                                                                                                                            |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Raw data directory | `src/tmp/results/liblinear-5.13.0-rc6nomad/results-liblinear/output.log`, `src/tmp/results/liblinear-5.13.0-rc6tpp/results-liblinear/output.log`, `src/tmp/results/liblinear-5.15.19-htmm/results-liblinear/output.log`                          |
| How to interpret   | For each log file, you will find a line  like `execution time 15084.021263 (s)`. Then its speed is `1/15084.021263`. When you compare across multiple kernel and platforms, divide the speed by the slowest one, then you get data in figure 12. |
| Plot path | `src/post_processing/tmp/liblinear.png` |

## License

[GPLv3](LICENSE)