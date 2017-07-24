EXTRA_CFLAGS = -Wall -O -Wimplicit-function-declaration -Wno-unused-result
obj-m := driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	cc     user.c   -o user

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f user