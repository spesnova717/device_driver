/* vmeput.c */
/* Created by Enomoto Sanshiro on 17 July 2000. */
/* Updated by Enomoto Sanshiro on 27 October 2005. */
/* Last updated by Enomoto Sanshiro on 15 July 2010. */


#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "vmedrv.h"


int main(int argc, char** argv)
{
    int fd;
    char device_file[1024];
    unsigned address, value;

    if (
	(argc < 4) ||
	(sscanf(argv[1], "%s", device_file) != 1) ||
	(sscanf(argv[2], "%x", &address) != 1) ||
	(sscanf(argv[3], "%x", &value) != 1)
    ){
        fprintf(stderr, "Usage: %s DEVICE_FILE ADDRESS VALUE\n", argv[0]);
        fprintf(stderr, "  ex) %s /dev/vmedrv32d32 0x02000000 0xff00ff00\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    if ((fd = open(device_file, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }
    
    if (lseek64(fd, address, SEEK_SET) == -1) {
	perror("ERROR: lseek64()");
	exit(EXIT_FAILURE);
    }

    if (strstr(device_file, "d16") != NULL) {
	unsigned short word = value;
	if (write(fd, &word, sizeof(word)) == -1) {
	    perror("ERROR: write()");
	    exit(EXIT_FAILURE);
	}
    }
    else {
	unsigned int word = value;
	if (write(fd, &word, sizeof(word)) == -1) {
	    perror("ERROR: write()");
	    exit(EXIT_FAILURE);
	}
    }

    close(fd);

    return 0;
}
