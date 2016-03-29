#!/bin/sh
# 获取板子相关信息
board_name=`uname -a | awk '{print $2}'`
kernel_version=`uname -a | awk '{print $3}'`
cpu_arch=`cat /proc/cpuinfo | grep "Processor" | awk 'BEGIN {FS=":"} {print $2}'`
hardware_serial=`cat /proc/cpuinfo | grep "Serial" | awk 'BEGIN {FS=":"} {print $2}'`

tmpnum1=`df -h | grep "rootfs" | awk '{print $1" "$2}'| uniq | awk '{print $2}' | sed 's/M//g'`
tmpnum2=`df -h | grep "opt" | awk '{print $1" "$2}'| uniq | awk '{print $2}' | sed 's/M//g'`

disk_total=`echo $tmpnum1 $tmpnum2 | awk '{print($1 + $2)}'`
mem_total=`free -m| grep "Mem" | awk '{print $2}'`

echo "cpu arch: "$cpu_arch
echo "disk size: "$disk_total"M"
echo "board name: "$board_name
echo "memory size: "$mem_total"M"
echo "kernel version: "$kernel_version
echo "hardware serial: "$hardware_serial

