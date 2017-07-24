/* test-interrupter-SMP.c */

/* Input NIM pulses into "SWITCH" connector (bottom left) to */
/* generate VME interrupts. IRQ and vector are software configurable. */


#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#define TEST_INTERRUPTER_DEV_FILE "/dev/vmedrv32d32"

static void* interrupter_mapped_address;
static int interrupter_map_base;
static int interrupter_map_size;


void test_interrupter_enable(int fd, int base_address, int irq, int vector) 
{
    interrupter_map_base = base_address + 0x180000;
    interrupter_map_size = 0x1000;

    if (sizeof(int) != 4) {
        fprintf(stderr, "ERROR: mmap(): wrong word size\n");
	exit(EXIT_FAILURE);
    }

    interrupter_mapped_address = mmap(
	0, interrupter_map_size, PROT_WRITE, MAP_SHARED, 
	fd, interrupter_map_base
    );
    if (interrupter_mapped_address == MAP_FAILED) {
	perror("ERROR: mmap()");
	exit(EXIT_FAILURE);
    }
    
    *((int*) (interrupter_mapped_address + (off_t) 0x0020)) = vector;
    *((int*) (interrupter_mapped_address + (off_t) 0x001c)) = 0x00f0 | irq;
}

void test_interrupter_disable(void)
{
    munmap(interrupter_mapped_address, interrupter_map_size);
}

void test_interrupter_clear(void)
{
    *((int*) (interrupter_mapped_address + (off_t) 0x001c)) |= 0x00f0;
}



static int interrupter_fd;

void test_interrupter_start(int base_address, int irq, int vector) 
{
    if ((interrupter_fd = open(TEST_INTERRUPTER_DEV_FILE, O_RDWR)) == -1) {
	perror("ERROR: open()");
	exit(EXIT_FAILURE);
    }
    test_interrupter_enable(interrupter_fd, base_address, irq, vector);
}

void test_interrupter_stop(void)
{
    test_interrupter_disable();
    close(interrupter_fd);
}
