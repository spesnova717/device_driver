# Makefile for vmedrv
# Created by Enomoto Sanshiro on 28 November 1999.
# Last updated by Enomoto Sanshiro on 27 December 1999.


CC = gcc
CPPFLAGS = -O -Wall -I/usr/include
DEBUGFLAGS = -I/usr/include -DTRACE_CONFIG -DTRACE_INTERRUPT -DTRACE_DMA


vmedrv.o: vmedrv.c vmedrv.h vmedrv_params.h vmedrv_conf.h
	$(CC) $(CPPFLAGS) -c $< 
	cp -p vmedrv.h ..


clean:
	rm -f *.o


install:
	/sbin/insmod vmedrv
	./install.sh


uninstall:
	/sbin/rmmod vmedrv
	rm -f /dev/vmedrv
	rm -f /dev/vmedrv??d??
	rm -f /dev/vmedrv??d??dma
