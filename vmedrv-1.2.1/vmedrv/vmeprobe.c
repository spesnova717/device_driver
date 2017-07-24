/* vmeprobe.c */
/* Created by Enomoto Sanshiro on 17 October 2005. */
/* Last updated by Enomoto Sanshiro on 17 October 2005. */


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
#include "vmedrv.h"


#define DEV_FILE "/dev/vmedrv32d32"

int main(int argc, char** argv)
{
    int fd;
    char excess[32];
    vmedrv_word_access_t word_access;
    unsigned long address;

    if (
	(argc < 2) ||
	(sscanf(argv[1], "%lx%s", &address, excess) != 1)
    ){
        fprintf(stderr, "Usage: %s ADDRESS \n", argv[0]);
        fprintf(stderr, "  ex) %s 0x02000000\n", argv[0]);
	exit(EXIT_FAILURE);
    }
        
    if ((fd = open(DEV_FILE, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }
    
    word_access.address = address;
    if (ioctl(fd, VMEDRV_IOC_PROBE, &word_access) == -1) {
	printf("failure: %s\n", strerror(errno));
    }
    else {
	printf("success: 0x%08lx\n", word_access.data);
    }

    close(fd);

    return 0;
}
