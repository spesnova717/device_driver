# Makefile for KEK-vmelib
# Created by Enomoto Sanshiro on 18 October 2005.  
# Last updated by Enomoto Sanshiro on 18 October 2005.  


VMEDRV_PATH=..
SAMPLES = map-test dma-test intwait-test intnowait-test

CC = gcc
CFLAGS += -g -Wall -I$(VMEDRV_PATH) -I.
LDFLAGS += -g -Wall -L.


all: libvmelib.a $(SAMPLES)


libvmelib.a: vmelib.o
	ar rcv libvmelib.a vmelib.o
	ranlib libvmelib.a

vmelib.o: vmelib.c vmelib.h


map-test: map-test.o libvmelib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmelib

dma-test: dma-test.o libvmelib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmelib

intwait-test: intwait-test.o libvmelib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmelib

intnowait-test: intnowait-test.o libvmelib.a
	$(CC) $(LDFLAGS) -o $@ $@.o -lvmelib


.c.o:
	$(CC) $(CFLAGS) -c $< 


clean:
	rm -f *.o
	rm -f libvmelib.a
	rm -f $(SAMPLES)
