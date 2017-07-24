/* vmemap.c */
/* Created by Enomoto Sanshiro on 10 December 1999. */
/* Last updated by Enomoto Sanshiro on 10 December 1999. */


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

#if 0
#include <asm/page.h>
#else
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#endif


#define DEV_FILE "/dev/vmedrv32d32"
#define MAX_SIZE 0x1000000

void memcpy_32(void* dest, const void* src, int size);
void dump_to_screen(const void* dump, int size, int address);


int main(int argc, char** argv)
{
    int fd;
    int address, size;
    void* mapped_address;
    off_t map_base, offset;
    size_t map_size;
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
    
    /* align the mapping base address to MMU page boundary */
    map_base = address & PAGE_MASK;
    offset = address - map_base;
    map_size = size + offset;

    mapped_address = mmap(0, map_size, PROT_WRITE, MAP_SHARED, fd, map_base);
    if (mapped_address == MAP_FAILED) {
	perror("ERROR: mmap()");
	exit(EXIT_FAILURE);
    }
    mapped_address += offset;

    memcpy_32(buffer, mapped_address, size);
    dump_to_screen(buffer, size, address);

    printf("mapped address: %p\n", mapped_address);
    printf("read size: 0x%x\n", size);

    munmap(mapped_address - offset, map_size);
    close(fd);

    return 0;
}


void memcpy_32(void* dest, const void* src, int size)
{
    int count;
    unsigned word;

    for (count = 0; count < size / 4; count++) {
        word = ((unsigned*) src)[count];
	((unsigned*) dest)[count] = word;
    } 
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
