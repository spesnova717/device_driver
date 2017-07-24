/* vmeint.c */
/* Created by Enomoto Sanshiro on 16 December 1999. */
/* Last updated by Enomoto Sanshiro on 14 October 2005. */


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


//#include "test-interrupter-RPV130.c"
#include "test-interrupter-SMP.c"

#define USE_AUTOCLEAR 1
#define USE_8BIT_VECTOR 1


void on_catch_signal(int signal_id);

int main(int argc, char** argv)
{
    int fd;
    int base_address, irq, vector;
    struct vmedrv_interrupt_property_t interrupt_property;
    char excess[32];

    if (
	(argc < 4) ||
	(sscanf(argv[1], "%x%s", &base_address, excess) != 1) ||
	(sscanf(argv[2], "%d%s", &irq, excess) != 1) ||
	(sscanf(argv[3], "%x%s", &vector, excess) != 1)
    ){
        fprintf(stderr, "Usage: %s BASE_ADDRESS IRQ VECTOR\n", argv[0]);
        fprintf(stderr, "  ex) %s 0x8000 3 0xfff0\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    if ((fd = open(TEST_INTERRUPTER_DEV_FILE, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }

    signal(SIGPOLL, on_catch_signal);
    test_interrupter_enable(fd, base_address, irq, vector);

    interrupt_property.irq = irq;
    interrupt_property.vector = vector;
    interrupt_property.signal_id = SIGPOLL;
    if (ioctl(fd, VMEDRV_IOC_REGISTER_INTERRUPT, &interrupt_property) == -1) {
	perror("ERROR: ioctrl(REGISTER_INTERRUPT)");
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

    printf("press <ENTER> to exit\n");

    if (ioctl(fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
	perror("ERROR: ioctrl(ENABLE_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    (void) getchar();

    if (ioctl(fd, VMEDRV_IOC_DISABLE_INTERRUPT) == -1) {
	perror("ERROR: ioctrl(DISABLE_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    if (ioctl(fd, VMEDRV_IOC_UNREGISTER_INTERRUPT, &interrupt_property) == -1) {
	perror("ERROR: ioctrl(UNREGISTER_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    test_interrupter_disable();
    close(fd);

    return 0;
}


void on_catch_signal(int signal_id)
{
    static int count = 0;

    printf("%d: VME interrupt is handled.\n", ++count);
    test_interrupter_clear();
}
