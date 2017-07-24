/* dma-test.c */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 26 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vmelib.h>


#define ACCESS_MODE VME_A32D32MODE
#define ADDRESS 0x02000000
#define SIZE 0x1000

void memcpy_32(void* dest, const void* src, int size);
void dump_to_screen(const void* dump, int size, int address);


int main(int argc, char** argv)
{
    int read_size;
    char buffer[SIZE];

    if (vme_dmaopen(ACCESS_MODE, MEMORY) != 0) {
	perror("ERROR: vme_dmaopen()");
	exit(EXIT_FAILURE);
    }

    if ((read_size = vme_dmaread(ADDRESS, buffer, SIZE)) != SIZE) {
	perror("ERROR: vme_dmaread()");
	exit(EXIT_FAILURE);
    }

    dump_to_screen(buffer, read_size, ADDRESS);

    if (vme_dmaclose(ACCESS_MODE) != 0) {
	perror("ERROR: vme_dmaclose()");
	exit(EXIT_FAILURE);
    }

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
