/* vmeintcheck.c */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 18 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "vmedrv.h"


#include "test-interrupter-RPV130.c"
//#include "test-interrupter-SMP.c"


#define N_REPEATS 128
#define TIMEOUT_SEC 3


void enable_module_interrupt(int fd, int base_address, int irq, int vector) ;
void on_catch_signal(int signal_id);


int main(int argc, char** argv)
{
    int fd;
    int base_address, irq, vector;
    int result;
    int i;
    struct vmedrv_interrupt_property_t interrupt_property;
    char excess[32];

    if (
	(argc < 4) ||
	(sscanf(argv[1], "%x%s", &base_address, excess) != 1) ||
	(sscanf(argv[2], "%d%s", &irq, excess) != 1) ||
	(sscanf(argv[3], "%x%s", &vector, excess) != 1)
    ){
        fprintf(stderr, "Usage: %s BASE IRQ VECTOR\n", argv[0]);
        fprintf(stderr, "  ex) %s 0x8000 3 0xfff0\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    if ((fd = open(TEST_INTERRUPTER_DEV_FILE, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }

    test_interrupter_enable(fd, base_address, irq, vector);

    interrupt_property.irq = irq;
    interrupt_property.vector = vector;
    interrupt_property.signal_id = 0;
    if (ioctl(fd, VMEDRV_IOC_REGISTER_INTERRUPT, &interrupt_property) == -1) {
	perror("ERROR: ioctl(REGISTER_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    if (ioctl(fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
	perror("ERROR: ioctl(ENABLE_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    interrupt_property.timeout = TIMEOUT_SEC;
    for (i = 0; i < N_REPEATS; i++) {
	result = ioctl(fd, VMEDRV_IOC_CHECK_INTERRUPT, &interrupt_property);
	if (result < 0) {
	    perror("ERROR: ioctl(CHECK_INTERRUPT)");
	    exit(EXIT_FAILURE);
	}

	if (result > 0) {
	    putchar('0' + result);
	    ioctl(fd, VMEDRV_IOC_CLEAR_INTERRUPT, &interrupt_property);
	    test_interrupter_clear();
	}
	else {
	    putchar('.');
	}
	usleep(100000);
	fflush(stdout);
    }
    putchar('\n');

    if (ioctl(fd, VMEDRV_IOC_DISABLE_INTERRUPT) == -1) {
	perror("ERROR: ioctl(DISABLE_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    if (ioctl(fd, VMEDRV_IOC_UNREGISTER_INTERRUPT, &interrupt_property) == -1) {
	perror("ERROR: ioctl(UNREGISTER_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    test_interrupter_disable();
    close(fd);

    return 0;
}
