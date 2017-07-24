/* vmelib.c */
/* KEK VMELIB compatible interface to vmedrv */
/* Created by Enomoto Sanshiro on 18 October 2005. */
/* Last updated by Enomoto Sanshiro on 18 October 2005. */


#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/page.h>
#include "vmedrv.h"
#include "vmelib.h"


#define mmap_table_size 1024
#define intr_table_size  10

static int mmap_count = 0;
static int mmap_fd_table[mmap_table_size];
static caddr_t mmap_addr_table[mmap_table_size];
static off_t mmap_offset_table[mmap_table_size];
static size_t mmap_size_table[mmap_table_size];

static int dma_fd = -1;

static int intr_fd_table[intr_table_size];
static int intr_vector_table[intr_table_size];

static const char* device_file[] = {
    "/dev/vmedrv16d16",
    "/dev/vmedrv16d32",
    "/dev/vmedrv24d16",
    "/dev/vmedrv24d32",
    "/dev/vmedrv32d16",
    "/dev/vmedrv32d32"
};

static const char* dma_device_file[] = {
    "/dev/vmedrv16d16",
    "/dev/vmedrv16d32",
    "/dev/vmedrv24d16dma",
    "/dev/vmedrv24d32dma",
    "/dev/vmedrv32d16dma",
    "/dev/vmedrv32d32dma"
};


caddr_t vme_mapopen(int mode, off_t vmeaddr, size_t size)
{
    int fd;
    volatile caddr_t mapped_address;
    off_t base;
    off_t offset;

    if (mmap_count >= mmap_table_size) {
	errno = ENOMEM;
	return (caddr_t) -1;
    }

    if ((fd = open(device_file[mode], O_RDWR)) == -1) {
	return (caddr_t) -1;
    }

    base = vmeaddr & PAGE_MASK;
    offset = vmeaddr - base;
    size += offset;

    mapped_address = mmap(0, size, PROT_WRITE, MAP_SHARED, fd, base);
    if (mapped_address == MAP_FAILED) {
	close(fd);
	return (caddr_t) -1;
    }
    mapped_address += offset;

    mmap_fd_table[mmap_count] = fd;
    mmap_addr_table[mmap_count] = mapped_address;
    mmap_offset_table[mmap_count] = offset;
    mmap_size_table[mmap_count] = size;
    mmap_count++;

    return mapped_address;
}

int vme_mapclose(caddr_t addr, int mode)
{
    int index;
    for (index = 0; index < mmap_count; index++) {
	if (mmap_addr_table[index] == addr) {
	    break;
	}
    }
    if (index == mmap_count) {
	errno = ENXIO;
	return errno;
    }

    if (mmap_fd_table[index] < 0) {
	errno = ENXIO;
	return errno;
    }

    mmap_addr_table[index] -= mmap_offset_table[index];

    munmap(mmap_addr_table[index], mmap_size_table[index]);
    close(mmap_fd_table[index]);

    mmap_fd_table[index] = -1;

    return 0;
}

void vme_mapprobe(off_t addr, size_t size, int mode)
{
    int fd;
    vmedrv_word_access_t word_access;

    if ((fd = open(device_file[mode], O_RDWR)) == -1) {
	abort();
    }

    word_access.address = addr;
    if (ioctl(fd, VMEDRV_IOC_PROBE, &word_access) == -1) {
	close(fd);
	abort();
    }

    close(fd);
}



int vme_dmaopen(int mode, int dma_dev)
{
    if (dma_fd >= 0) {
	errno = EBUSY;
	return -1;
    }

    if ((dma_fd = open(dma_device_file[mode], O_RDWR)) == -1) {
	return -1;
    }

    return 0;
}

int vme_dmaclose(int mode)
{
    if (dma_fd < 0) {
	errno = ENODEV;
	return -1;
    }

    close(dma_fd);
    dma_fd = -1;

    return 0;
}

int vme_dmaread(int vmeaddr, void *buf, size_t count)
{
    int read_size;
    if (lseek(dma_fd, vmeaddr, SEEK_SET) == -1) {
	return errno;
    }

    if ((read_size = read(dma_fd, buf, count)) == -1) {
	return errno;
    }

    return read_size;
}

int vme_dmawrite( int vmeaddr, void *buf, size_t count)
{
    int written_size;
    if (lseek(dma_fd, vmeaddr, SEEK_SET) == -1) {
	return errno;
    }

    if ((written_size = write(dma_fd, buf, count)) == -1) {
	return errno;
    }

    return written_size;
}



int vme_intopen(int vector, int* intrno)
{
    int fd;
    struct vmedrv_interrupt_property_t interrupt_property;

    if ((*intrno < 0) || (*intrno >= intr_table_size)) {
	errno = ENXIO;
	return errno;
    }

    if ((fd = open("/dev/vmedrv", O_RDWR)) == -1) {
	return errno;
    }

    interrupt_property.irq = *intrno;
    interrupt_property.vector = vector;
    interrupt_property.signal_id = 0;
    if (ioctl(fd, VMEDRV_IOC_REGISTER_INTERRUPT, &interrupt_property) == -1) {
	close(fd);
	return errno;
    }

    intr_fd_table[*intrno] = fd;
    intr_vector_table[*intrno] = vector;

    return 0;
}

int vme_intclose(int intrno)
{
    if ((intrno < 0) || (intrno >= intr_table_size)) {
	errno = ENXIO;
	return errno;
    }
    if (intr_fd_table[intrno] < 0) {
	errno = ENXIO;
	return errno;
    }

    close(intr_fd_table[intrno]);

    return 0;
}

int vme_intenable(int intrno)
{
    int fd;

    fd = intr_fd_table[intrno];
    if (ioctl(fd, VMEDRV_IOC_ENABLE_INTERRUPT) == -1) {
	return errno;
    }

    return 0;
}

int vme_intdisable(int intrno)
{
    int fd;

    fd = intr_fd_table[intrno];
    if (ioctl(fd, VMEDRV_IOC_DISABLE_INTERRUPT) == -1) {
	return errno;
    }

    return 0;
}

int vme_intwait(int intrno, int* interrupt_count, int timeout)
{
    int fd;
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    *interrupt_count = 0;

    fd = intr_fd_table[intrno];
    interrupt_property.irq = intrno;
    interrupt_property.vector = intr_vector_table[intrno];
    interrupt_property.timeout = timeout;

    result = ioctl(fd, VMEDRV_IOC_WAIT_FOR_INTERRUPT, &interrupt_property);
    if (result <= 0) {
	return errno;
    }
    *interrupt_count = 1;

    return 0;
}

int vme_intnowait(int intrno, int* interrupt_count)
{
    if (vme_intcheck(intrno, interrupt_count) == -1) {
	return errno;
    }

    if (*interrupt_count > 0) {
	if (vme_intclear(intrno) == -1) {
	    return errno;
	}
    }

    return 0;
}

int vme_intclear(int intrno)
{
    int fd;
    struct vmedrv_interrupt_property_t interrupt_property;

    fd = intr_fd_table[intrno];
    interrupt_property.irq = intrno;
    interrupt_property.vector = intr_vector_table[intrno];

    if (ioctl(fd, VMEDRV_IOC_CLEAR_INTERRUPT, &interrupt_property) == -1) {
	return errno;
    }

    return 0;
}

int vme_intcheck(int intrno, int* interrupt_count)
{
    int fd;
    struct vmedrv_interrupt_property_t interrupt_property;
    int result;

    *interrupt_count = 0;

    fd = intr_fd_table[intrno];
    interrupt_property.irq = intrno;
    interrupt_property.vector = intr_vector_table[intrno];

    result = ioctl(fd, VMEDRV_IOC_CHECK_INTERRUPT, &interrupt_property);
    if (result == -1) {
	return errno;
    }
    else {
	*interrupt_count = result;
    }

    return 0;
}
