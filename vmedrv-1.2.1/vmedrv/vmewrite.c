/* vmewrite.c */
/* Created by Enomoto Sanshiro on 19 May 2000. */
/* Last updated by Enomoto Sanshiro on 15 July 2010. */


#define _LARGEFILE64_SOURCE

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
#define MAX_SIZE 0x1000000


void memfill_32(void* dest, int size, int word);


int main(int argc, char** argv)
{
    int fd;
    int address, size, value;
    int written_size;
    static char buffer[MAX_SIZE];
    char excess[32];

    if (
	(argc < 4) ||
	(sscanf(argv[1], "%x%s", &address, excess) != 1) ||
	(sscanf(argv[2], "%x%s", &size, excess) != 1) ||
	(sscanf(argv[3], "%x%s", &value, excess) != 1)
    ){
        fprintf(stderr, "Usage: %s ADDRESS SIZE VALUE\n", argv[0]);
        fprintf(stderr, "  ex) %s 0x02000000 0x10000 0x12345678\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if (size > MAX_SIZE) {
        fprintf(stderr, "ERROR: too large size (MAX_SIZE = 0x%x)\n", MAX_SIZE);
	exit(EXIT_FAILURE);
    }
        
    memfill_32(buffer, size, value);

    if ((fd = open(DEV_FILE, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }
    
    if (lseek64(fd, address, SEEK_SET) == -1) {
	perror("ERROR: lseek64()");
	exit(EXIT_FAILURE);
    }

    if ((written_size = write(fd, buffer, size)) == -1) {
	perror("ERROR: write()");
	exit(EXIT_FAILURE);
    }

    printf("written size: 0x%x\n", written_size);

    close(fd);

    return 0;
}


void memfill_32(void* dest, int size, int word)
{
    int count;

    for (count = 0; count < size / 4; count++) {
        ((unsigned*) dest)[count] = word;
    }
}
