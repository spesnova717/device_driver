/* map-test.c */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 26 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vmeslib.h>


#define ACCESS_MODE VME_A32D32
#define ADDRESS 0x02000000
#define SIZE 0x1000

void memcpy_32(void* dest, const void* src, int size);
void dump_to_screen(const void* dump, int size, int address);


int main(int argc, char** argv)
{
    VMEMAP* vmemap;
    char buffer[SIZE];

    if ((vmemap = vme_mapopen(ACCESS_MODE, ADDRESS, SIZE)) == NULL) {
	perror("ERROR: mapopen()");
	exit(EXIT_FAILURE);
    }
    
    memcpy_32(buffer, vme_mapbase(vmemap), SIZE);
    dump_to_screen(buffer, SIZE, ADDRESS);

    if (vme_mapclose(vmemap) == -1) {
	perror("ERROR: mapclose()");
	exit(EXIT_FAILURE);
    }

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
