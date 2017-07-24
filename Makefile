EXTRA_CFLAGS = -Wall -O -Wimplicit-function-declaration -Wno-unused-result
obj-m := driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	cc     user.c   -o user

install: all
	install -m 644 virtex-7.rules /etc/udev/rules.d

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f user
	rm -rf *.o *.o.d *~ core .depend .*.cmd *.ko *.ko.unsigned *.mod.c .tmp_versions *.symvers .#* *.save *.bak Modules.* modules.order Module.markers *.bin