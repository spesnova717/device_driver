/* probe-test.c */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 26 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vmeslib.h>


#define ACCESS_MODE VME_A32D32
#define ADDRESS 0x01000000


int main(int argc, char** argv)
{
    if (vme_tryget(ACCESS_MODE, ADDRESS) == -1) {
	perror("vme_tryget()");
	exit(EXIT_FAILURE);
    }

    printf("success\n");

    return 0;
}
