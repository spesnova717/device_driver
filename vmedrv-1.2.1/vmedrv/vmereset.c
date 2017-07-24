/* vmereset.c */
/* Created by Enomoto Sanshiro on 6 March 2009. */
/* Last updated by Enomoto Sanshiro on 6 March 2009. */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "vmedrv.h"


int main(int argc, char** argv)
{
    int fd;

    if ((fd = open("/dev/vmedrv", O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }
    
    if (ioctl(fd, VMEDRV_IOC_RESET_ADAPTER) == -1) {
	perror("ERROR: ioctl()");
    }

    close(fd);

    return 0;
}
