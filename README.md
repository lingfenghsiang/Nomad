# Nomad

Implementation of paper:
Lingfeng Xiang, Zhen Lin, Weishu Deng, Hui Lu, Jia Rao, Yifan Yuan, Ren Wang. "Nomad: Non-Exclusive Memory Tiering via Transactional Page Migration", OSDI 2024


[![standard-readme compliant](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)

A standard style for README files

Your README file is normally the first entry point to your code. It should tell people why they should use your module, how they can install it, and how they can use it. Standardizing how you write your README makes creating and maintaining your READMEs easier. Great documentation takes work!

This repository contains:

1. [The specification](spec.md) for how a standard README should look.
2. A link to [a linter](https://github.com/RichardLitt/standard-readme-preset) you can use to keep your README maintained ([work in progress](https://github.com/RichardLitt/standard-readme/issues/5)).
3. A link to [a generator](https://github.com/RichardLitt/generator-standard-readme) you can use to create standard READMEs.
4. [A badge](#badge) to point to this spec.
5. [Examples of standard READMEs](example-readmes/) - such as this file you are reading.

Standard Readme is designed for open source libraries. Although it’s [historically](#background) made for Node and npm projects, it also applies to libraries in other languages and package managers.


## Table of Contents

- [Nomad](#nomad)
	- [Table of Contents](#table-of-contents)
	- [Prerequisites](#prerequisites)
	- [Usage](#usage)
		- [Compiling the code](#compiling-the-code)
	- [Matching paper results](#matching-paper-results)
	- [License](#license)

## Prerequisites

If you want to run Nomad in a virtual machine, you may need network access to the virtual machine.
```
sudo apt-get install qemu-kvm libvirt-daemon-system libvirt-clients bridge-utils
sudo adduser `id -un` libvirt
sudo adduser `id -un` kvm
```

## Usage

This is only a documentation package. You can print out [spec.md](spec.md) to your console:

```sh
$ standard-readme-spec
# Prints out the standard-readme spec
```

### Compiling the code

```
git clone https://github.com/lingfenghsiang/Nomad.git
docker run -v ./Nomad:/root/code -it --rm docklf/ubuntu20-kerncomp:aec-v0.2
```
In the docker container, please execute:
```
cd /root/code
patch -p1 < src/implementation_patches/nomad.patch
make menuconfig
make deb-pkg -j`nproc`
```

## Matching paper results

If your README is compliant with Standard-Readme and you're on GitHub, it would be great if you could add the badge. This allows people to link back to this Spec, and helps adoption of the README. The badge is **not required**.

[![standard-readme compliant](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)

To add in Markdown format, use this code:

```
[![standard-readme compliant](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)
```

## License

[GPLv3](LICENSE)