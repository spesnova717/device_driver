#!/bin/bash

# Load the FPGA PCIe driver
if [ `lsmod | grep -o FPGA` ]; then
	echo "FPGA driver has already been loaded. Doing nothing."
	exit
fi
insmod FPGA.ko

#Find what major device number was assigned from /proc/devices
majorNum=$( awk '{ if ($2 ~ /fpga/) print $1}' /proc/devices )

if [ -z "$majorNum" ]; then
	echo "Unable to find the FPGA device!"
	echo "Did the driver correctly load?"
else
	#Remove any stale device file
	if [ -e "/dev/fpga" ]; then
		rm -r /dev/fpga
	fi

	#Create a new one with full read/write permissions for everyone
	sudo mknod -m 666 /dev/fpga c $majorNum 0
fi


