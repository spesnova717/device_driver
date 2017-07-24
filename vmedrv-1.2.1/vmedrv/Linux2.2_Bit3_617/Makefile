# Makefile for vmedrv
# Created by Enomoto Sanshiro on 28 November 1999.
# Last updated by Enomoto Sanshiro on 11 November 2000.


KERNEL_INCLUDE_DIR = /usr/include
# KERNEL_INCLUDE_DIR = /usr/src/linux/include
# KERNEL_INCLUDE_DIR = /usr/src/kernel-source/include

USE_MODVERSIONS = 1
# USE_MODVERSIONS = 0


CC = gcc
CPPFLAGS = -O -Wall -I$(KERNEL_INCLUDE_DIR) -DUSE_MODVERSIONS=$(USE_MODVERSIONS)
DEBUGFLAGS = -DTRACE_CONFIG -DTRACE_MAP -DTRACE_INTERRUPT -DTRACE_DMA


vmedrv.o: vmedrv.c vmedrv.h vmedrv_params.h vmedrv_conf.h Makefile
	$(CC) $(CPPFLAGS) -c $< 
	cp -p vmedrv.h ..


clean:
	rm -f *.o


install:
	@if /sbin/insmod vmedrv.o; then \
		./install.sh; \
	fi


uninstall:
	/sbin/rmmod vmedrv
	rm -f /dev/vmedrv
	rm -f /dev/vmedrv??d??
	rm -f /dev/vmedrv??d??dma
