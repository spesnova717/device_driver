/* vmeread.c */
/* Created by Enomoto Sanshiro on 8 December 1999. */
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

void dump_to_screen(const void* dump, int size, int address);

int main(int argc, char** argv)
{
    int fd;
    int address, size;
    int read_size;
    static char buffer[MAX_SIZE];
    char excess[32];

    if (
	(argc < 3) ||
	(sscanf(argv[1], "%x%s", &address, excess) != 1) ||
	(sscanf(argv[2], "%x%s", &size, excess) != 1)
    ){
        fprintf(stderr, "Usage: %s ADDRESS SIZE\n", argv[0]);
        fprintf(stderr, "  ex) %s 0x02000000 0x10000\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if (size > MAX_SIZE) {
        fprintf(stderr, "ERROR: too large size (MAX_SIZE = 0x%x)\n", MAX_SIZE);
	exit(EXIT_FAILURE);
    }
        
    if ((fd = open(DEV_FILE, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }
    
    if (lseek64(fd, address, SEEK_SET) == -1) {
	perror("ERROR: lseek64()");
	exit(EXIT_FAILURE);
    }

    if ((read_size = read(fd, buffer, size)) == -1) {
	perror("ERROR: read()");
	exit(EXIT_FAILURE);
    }

    dump_to_screen(buffer, read_size, address);

    printf("read size: 0x%x\n", read_size);

    close(fd);

    return 0;
}


void dump_to_screen(const void* data, int size, int base_address) 
{
    unsigned long address = base_address;
    unsigned index, byte;
    char string[32] = "";
    string[16] = '\0';

    for (index = 0; index < size; index++) {
	if (address % 16 == 0) {
	    printf("%04lx %04lx:  ", address >> 16, address & 0x0000ffff);
	}
	else if (address % 8 == 0) {
	    printf("  ");
	}

	byte = ((unsigned char*) data)[index];
	printf("%02x ", byte);
	string[address % 16] = isprint(byte) ? byte : '.';
	address++;

	if (address % 16 == 0) {
	    printf("  %s\n", string);
	}
    }
    putchar('\n');
}
