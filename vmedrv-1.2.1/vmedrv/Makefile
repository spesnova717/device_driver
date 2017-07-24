# Makefile for vmedrv libraries
# Created by Enomoto Sanshiro on 8 December 1999.  
# Last updated by Enomoto Sanshiro on 20 December 1999.


CC = gcc
CFLAGS = -g -Wall -I/usr/include
CXX = g++
CXXFLAGS = -g -Wall -I/usr/include

TARGETS = vmeread vmewrite vmedmaread vmedmawrite vmeint vmeintwait vmeintcheck vmepoll vmeprobe vmereset vmeput vmeget vmedeb vmemap


all: $(TARGETS)


vmeread: vmeread.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmewrite: vmewrite.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmedmaread: vmedmaread.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmedmawrite: vmedmawrite.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmemap: vmemap.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmeint: vmeint.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmeintwait: vmeintwait.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmeintcheck: vmeintcheck.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmepoll: vmepoll.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmeprobe: vmeprobe.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmereset: vmereset.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmeput: vmeput.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmeget: vmeget.o
	$(CC) $(CFLAGS) -o $@ $@.o

vmedeb: vmedeb.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o

pagesize: pagesize.o
	$(CC) $(CFLAGS) -o $@ $@.o


.c.o:
	$(CC) $(CFLAGS) -c $< 


.cc.o:
	$(CXX) $(CXXFLAGS) -c $< 


clean:
	rm -f *.o
	rm -f $(TARGETS)

