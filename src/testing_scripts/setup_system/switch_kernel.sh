#!/bin/bash

# Check if the script is run as root
if [ "$(id -u)" -ne "0" ]; then
    echo "This script must be run as root" 1>&2
    exit 1
fi

# Define the directory where the kernel images are stored
KERNEL_DIR="/boot"

# List available kernel versions
echo "Available kernel versions:"
ls $KERNEL_DIR/vmlinuz-* | awk -F/ '{print $NF}' | sed 's/vmlinuz-//' | sort -V
echo ""

# Prompt the user to select a kernel version
read -p "Enter the kernel version you want to switch to: " kernel_version

# Check if the selected kernel version exists
if [ ! -e "$KERNEL_DIR/vmlinuz-$kernel_version" ]; then
    echo "Kernel version $kernel_version does not exist"
    exit 1
fi

MID=`awk '/Advanced options.*/{print $(NF-1)}' /boot/grub/grub.cfg`
MID="${MID//\'/}"

KID=`awk -v kern="with Linux $kernel_version" '$0 ~ kern && !/recovery/ { print $(NF - 1) }' /boot/grub/grub.cfg`
KID="${KID//\'/}"

# update-grub

sed -i "s/GRUB_DEFAULT=.*/GRUB_DEFAULT=\"$MID>$KID\"/" /etc/default/grub


if [ $kernel_version = "5.15.19-htmm" ];then
    sed -i '/^[^#].*memmap/ s/^/#/' /etc/default/grub   
else
    sed -i '/^#.*memmap/ s/^#//' /etc/default/grub
fi


update-grub

echo -e "\e[31mPlease reboot machine\e[0m"

