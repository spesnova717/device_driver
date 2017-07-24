/* pagesize.c */


#include <stdio.h>
#include <asm/page.h>


int main(void)
{
    printf("Page Size: %ld bytes ", PAGE_SIZE);
    printf("(0x%4lx bytes in HEX)\n", PAGE_SIZE);

    return 0;
}
