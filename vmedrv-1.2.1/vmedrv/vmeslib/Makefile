# Makefile for vmeslib
# Created by Enomoto Sanshiro on 18 October 2005.  
# Last updated by Enomoto Sanshiro on 18 October 2005.  


VMEDRV_PATH=..
SAMPLES = map-test dma-test intwait-test intnowait-test probe-test

CC = gcc
CFLAGS += -g -Wall -I$(VMEDRV_PATH) -I.
LDFLAGS += -g -Wall -L.


all: libvmeslib.a $(SAMPLES)


libvmeslib.a: vmeslib.o
	ar rcv libvmeslib.a vmeslib.o
	ranlib libvmeslib.a

vmeslib.o: vmeslib.c vmeslib.h


map-test: map-test.o libvmeslib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmeslib

dma-test: dma-test.o libvmeslib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmeslib

intwait-test: intwait-test.o libvmeslib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmeslib

intnowait-test: intnowait-test.o libvmeslib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmeslib

probe-test: probe-test.o libvmeslib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmeslib


.c.o:
	$(CC) $(CFLAGS) -c $< 


clean:
	rm -f *.o
	rm -f libvmeslib.a
	rm -f $(SAMPLES)
