obj-m := FPGA.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	cc     user_keisoku.c   -o user_keisoku.out
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	rm -f user_keisoku.out
