/* intwait-test.c */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 26 October 2005. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vmeslib.h>


#include "test-interrupter-RPV130.c"

#define BASE_ADDRESS 0x8000
#define INTR_NO 3
#define VECTOR 0xfff0

#define N_REPEATS 32
#define TIMEOUT_SEC 3
#define USE_8BIT_VECTOR 1
#define USE_AUTODISABLE 1

int main(int argc, char** argv)
{
    VMEINT* vmeint;
    int interrupt_count;
    int i;

    test_interrupter_start(BASE_ADDRESS, INTR_NO, VECTOR);

    if ((vmeint = vme_intopen(INTR_NO, VECTOR)) == NULL) {
	perror("ERROR: vme_intopen()");
	exit(EXIT_FAILURE);
    }
    if (vme_intenable(vmeint) != 0) {
	perror("ERROR: vme_intenable()");
	exit(EXIT_FAILURE);
    }

#ifdef USE_8BIT_VECTOR
    if (vme_intsetvectormask(vmeint, 0x00ff) != 0) {
	perror("ERROR: vme_intsetvectormask()");
	exit(EXIT_FAILURE);
    }
#endif

#ifdef USE_AUTODISABLE
    if (vme_intsetautodisable(vmeint) != 0) {
	perror("ERROR: vme_intsetautodisable()");
	exit(EXIT_FAILURE);
    }
#endif

    for (i = 0; i < N_REPEATS; i++) {
	if ((interrupt_count = vme_intwait(vmeint, TIMEOUT_SEC)) == -1) {
	    if (errno == EINTR) {
		printf("interrupted.\n");
	    }
	    else {
		perror("ERROR: vme_intwait()");
		exit(EXIT_FAILURE);
	    }
	}
	if (interrupt_count > 0) {
	    printf("VME interrupt handled: %d\n", interrupt_count);
	    test_interrupter_clear();
#ifdef USE_AUTODISABLE
	    if (vme_intenable(vmeint) != 0) {
		perror("ERROR: vme_intenable()");
		exit(EXIT_FAILURE);
	    }
#endif
	}
	else {
	    printf("timed out.\n");
	}
    }

    if (vme_intdisable(vmeint) != 0) {
	perror("ERROR: vme_intdisable()");
	exit(EXIT_FAILURE);
    }
    if (vme_intclose(vmeint) != 0) {
	perror("ERROR: vme_intclose()");
	exit(EXIT_FAILURE);
    }

    test_interrupter_stop();

    return 0;
}
