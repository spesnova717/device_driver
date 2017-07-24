/* intnowait-test.c */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 26 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vmelib.h>


#include "test-interrupter-RPV130.c"
//#include "test-interrupter-SMP.c"

#define BASE_ADDRESS 0x8000
#define INTR_NO 3
#define VECTOR 0xfff0

#define N_REPEATS 128
#define TIMEOUT_SEC 3


int main(int argc, char** argv)
{
    int intrno = INTR_NO;
    int interrupt_count;
    int i;

    test_interrupter_start(BASE_ADDRESS, INTR_NO, VECTOR);

    if (vme_intopen(VECTOR, &intrno) != 0) {
	perror("ERROR: vme_intopen()");
	exit(EXIT_FAILURE);
    }
    if (vme_intenable(intrno) != 0) {
	perror("ERROR: vme_intenable()");
	exit(EXIT_FAILURE);
    }

    for (i = 0; i < N_REPEATS; i++) {
	if (vme_intnowait(intrno, &interrupt_count) != 0) {
	    perror("ERROR: vme_intnowait()");
	    exit(EXIT_FAILURE);
	}

	if (interrupt_count > 0) {
	    putchar('0' + interrupt_count);
	    test_interrupter_clear();
	}
	else {
	    putchar('.');
	}

	fflush(stdout);
	usleep(100000);
    }
    putchar('\n');

    if (vme_intdisable(intrno) != 0) {
	perror("ERROR: vme_intdisable()");
	exit(EXIT_FAILURE);
    }
    if (vme_intclose(intrno) != 0) {
	perror("ERROR: vme_intclose()");
	exit(EXIT_FAILURE);
    }

    test_interrupter_stop();

    return 0;
}