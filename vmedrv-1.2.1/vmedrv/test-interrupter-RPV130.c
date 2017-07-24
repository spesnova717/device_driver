

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#define TEST_INTERRUPTER_DEV_FILE "/dev/vmedrv16d16"

static void* interrupter_mapped_address;
static int interrupter_map_base;
static int interrupter_map_size;


void test_interrupter_enable(int fd, int base_address, int irq, int vector) 
{
    interrupter_map_base = base_address;
    interrupter_map_size = 0x1000;

    if (sizeof(short) != 2) {
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
    
    /* clear */
    *((short*) (interrupter_mapped_address + (off_t) 0x000c)) |= 0x03;
    *((short*) (interrupter_mapped_address + (off_t) 0x000e)) |= 0x03;
    
    /* enable interrupt */
    *((short*) (interrupter_mapped_address + (off_t) 0x000c)) |= 0x50;
    *((short*) (interrupter_mapped_address + (off_t) 0x000e)) |= 0x50;
}

void test_interrupter_disable(void)
{
    munmap(interrupter_mapped_address, interrupter_map_size);
}

void test_interrupter_clear(void)
{
    /* clear */
    *((short*) (interrupter_mapped_address + (off_t) 0x000c)) |= 0x03;
    *((short*) (interrupter_mapped_address + (off_t) 0x000e)) |= 0x03;
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
