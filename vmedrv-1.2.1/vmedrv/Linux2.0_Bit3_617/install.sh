#!/bin/sh
# install script for vmedrv
# Created by Enomoto Sanshiro on 28 November 1999.
# Last updated by Enomoto Sanshiro on 27 December 1999.


module="vmedrv"
group="wheel"
mode="666"


# install module #
#/sbin/insmod $module $* || exit 1

# remove old node #
rm -f /dev/vmedrv
rm -f /dev/vmedrv??d??
rm -f /dev/vmedrv??d??dma

# get major id #
major=`awk '/vmedrv/{print $1}' /proc/devices`

# make new node #
mknod /dev/vmedrv c $major 0
mknod /dev/vmedrv16d16 c $major 1
mknod /dev/vmedrv16d32 c $major 2
mknod /dev/vmedrv24d16 c $major 3
mknod /dev/vmedrv24d32 c $major 4
mknod /dev/vmedrv32d16 c $major 5
mknod /dev/vmedrv32d32 c $major 6
mknod /dev/vmedrv24d16dma c $major 7
mknod /dev/vmedrv24d32dma c $major 8
mknod /dev/vmedrv32d16dma c $major 9
mknod /dev/vmedrv32d32dma c $major 10

# change group id #
#chgrp $group /dev/vmedrv
#chgrp $group /dev/vmedrv16d16
#chgrp $group /dev/vmedrv16d32
#chgrp $group /dev/vmedrv24d16
#chgrp $group /dev/vmedrv24d32
#chgrp $group /dev/vmedrv32d16
#chgrp $group /dev/vmedrv32d32
#chgrp $group /dev/vmedrv24d16dma
#chgrp $group /dev/vmedrv24d32dma
#chgrp $group /dev/vmedrv32d16dma
#chgrp $group /dev/vmedrv32d32dma

# change access mode #
chmod $mode /dev/vmedrv
chmod $mode /dev/vmedrv16d16
chmod $mode /dev/vmedrv16d32
chmod $mode /dev/vmedrv24d16
chmod $mode /dev/vmedrv24d32
chmod $mode /dev/vmedrv32d16
chmod $mode /dev/vmedrv32d32
chmod $mode /dev/vmedrv24d16dma
chmod $mode /dev/vmedrv24d32dma
chmod $mode /dev/vmedrv32d16dma
chmod $mode /dev/vmedrv32d32dma
