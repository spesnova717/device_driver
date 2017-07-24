/* vmepoll.c */
/* Created by Enomoto Sanshiro on 14 October 2005. */
/* Last updated by Enomoto Sanshiro on 14 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include "vmedrv.h"


#include "test-interrupter-RPV130.c"
//#include "test-interrupter-SMP.c"

#define N_REPEATS 32
#define TIMEOUT_SEC 3


int main(int argc, char** argv)
{
    int fd;
    int base_address, irq, vector;
    int result, i;
    struct vmedrv_interrupt_property_t interrupt_property;
    struct pollfd fd_set[2];
    char buffer[1024], excess[32];

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
    interrupt_property.timeout = 1;
    if (ioctl(fd, VMEDRV_IOC_REGISTER_INTERRUPT, &interrupt_property) == -1) {
	perror("ERROR: ioctl(REGISTER_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    if (ioctl(fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
	perror("ERROR: ioctl(ENABLE_INTERRUPT)");
	exit(EXIT_FAILURE);
    }

    /* stdin */
    fd_set[0].fd = 0;
    fd_set[0].events = POLLIN;
    /* VME */
    fd_set[1].fd = fd;
    fd_set[1].events = POLLIN;
    for (i = 0; i < N_REPEATS; i++) {
	errno = 0;
	if (poll(fd_set, 2, TIMEOUT_SEC * 1000) < 0) {
	    perror("ERROR: poll()");
	    exit(EXIT_FAILURE);
	}
	printf(".");
	fflush(stdout);

	if (fd_set[0].revents & POLLIN) {
	    printf("stdin: ");
	    fgets(buffer, sizeof(buffer), stdin);
	    printf("%s", buffer);
	}
	if (fd_set[1].revents & POLLIN) {
	    printf("VME: ");
	    result = ioctl(
		fd, VMEDRV_IOC_WAIT_FOR_INTERRUPT, &interrupt_property
	    );
	    if (result > 0) {
		printf("interrupt handled.\n");
		test_interrupter_clear();
	    }
	    else if (errno == ETIMEDOUT) {
		printf("timed out.\n");
	    }
	    else if (errno == EINTR) {
		printf("interrupted.\n");
	    }
	    else {
		perror("ERROR: ioctl(WAIT_FOR_INTERRUPT)");
		exit(EXIT_FAILURE);
	    }
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
