/* vmeintwait.c */
/* Created by Enomoto Sanshiro on 19 June 2001. */
/* Last updated by Enomoto Sanshiro on 14 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "vmedrv.h"


//#include "test-interrupter-RPV130.c"
#include "test-interrupter-SMP.c"


#define N_REPEATS 32
#define TIMEOUT_SEC 3
#define USE_AUTOCLEAR 1
#define USE_8BIT_VECTOR 1


void enable_module_interrupt(int fd, int base_address, int irq, int vector) ;


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

#ifdef USE_8BIT_VECTOR
    interrupt_property.vector_mask = 0x00ff;
    if (ioctl(fd, VMEDRV_IOC_SET_VECTOR_MASK, &interrupt_property) == -1) {
	perror("ERROR: ioctrl(SET_VECTOR_MASK)");
	exit(EXIT_FAILURE);
    }
#endif

#ifdef USE_AUTOCLEAR
    if (ioctl(fd, VMEDRV_IOC_SET_INTERRUPT_AUTODISABLE, &interrupt_property) == -1) {
	perror("ERROR: ioctrl(SET_INTERRUPT_AUTODISABLE)");
	exit(EXIT_FAILURE);
    }
#endif

    if (ioctl(fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
	perror("ERROR: ioctl(ENABLE_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    interrupt_property.timeout = TIMEOUT_SEC;
    for (i = 0; i < N_REPEATS; i++) {
	result = ioctl(fd, VMEDRV_IOC_WAIT_FOR_INTERRUPT, &interrupt_property);
	if (result > 0) {
	    printf("%d: VME interrupt handled.\n", i);
	    test_interrupter_clear();
#ifdef USE_AUTOCLEAR
	    if (ioctl(fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
		perror("ERROR: ioctl(ENABLE_INTERRUPT)");
		exit(EXIT_FAILURE);
	    }
#endif
	}
	else if (errno == ETIMEDOUT) {
	    printf("%d: timed out.\n", i);
	}
	else if (errno == EINTR) {
	    printf("%d: interrupted.\n", i);
	}
	else {
	    perror("ERROR: ioctl(WAIT_FOR_INTERRUPT)");
	    exit(EXIT_FAILURE);
	}
    }

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
