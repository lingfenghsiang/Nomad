# Source code folders

The source code include the following folders:


* docker_commands. This folder contains the commands to run in docker. They are about compiling the kernel and the statically compile the userspace microbenchmark.
* linux-5.15.19. This folder contains the code of Memtis. We came across a few bugs and we temporarily work around these bugs by hard code memory node informations. Note that it only runs on a machine that has a fast NUMA node (CPU + memory) and a CPU-less slower NUMA node (CXL memory or Persistent memory). Currently CXL memory access is still uncore event and cannot be simply imported by directly encode the hardware event. Therefore, tracking LLC misses to CXL memory yet remains to be done.
* nomad_module. Part of our implementation. We have some code in the kernel and part in a kernel module. We will compile that once we installed the Nomad kernel.
* userspace_programs. This folder contains the userspace micro benchmarks that intensively read and write memory. The code is statitally compiled, and you may run the compile the binary file on any Linux machine.
* implementation_patches. Both Nomad and TPP are based on Linux-5.13-rc6. We put the in-kernel part of Nomad and TPP as patches. To compile Nomad and TPP, you need to apply these patches.
* vm_scripts. Some util scripts if you want to run Nomad in a virtual machine.
* kernel_config. The config file for Nomad, TPP and Memtis.
* testing_scripts. The scripts to set up environments and run tests.
* linux-5.13-rc6. The original code of Linux v5.13-rc6.
* memtis_userspace. Essential scripts and programs for Memtis, sourced from the Memtis repository.